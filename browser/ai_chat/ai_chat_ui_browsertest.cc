/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/test/bind.h"
#include "brave/browser/ui/webui/ai_chat/ai_chat_ui.h"
#include "brave/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "brave/components/text_recognition/common/buildflags/buildflags.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "printing/buildflags/buildflags.h"
#include "ui/compositor/compositor_switches.h"

namespace {

constexpr char kEmbeddedTestServerDirectory[] = "leo";
}  // namespace

class AIChatUIBrowserTest : public InProcessBrowserTest {
 public:
  AIChatUIBrowserTest() : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(&https_server_);

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    test_data_dir = base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    test_data_dir = test_data_dir.AppendASCII(kEmbeddedTestServerDirectory);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(https_server_.Start());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
    command_line->AppendSwitch(::switches::kEnablePixelOutputInTests);
#endif
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  content::WebContents* GetActiveWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void NavigateURL(const GURL& url) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    ASSERT_TRUE(WaitForLoadStop(GetActiveWebContents()));
  }

  void CreatePrintPreview(ai_chat::AIChatUIPageHandler* handler) {
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
    handler->MaybeCreatePrintPreview();
#endif
  }

  ai_chat::AIChatUIPageHandler* OpenAIChatSidePanel() {
    auto* side_panel_ui = SidePanelUI::GetSidePanelUIForBrowser(browser());
    side_panel_ui->Show(SidePanelEntryId::kChatUI);
    auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
    auto* side_panel = browser_view->unified_side_panel();
    auto* ai_chat_side_panel =
        side_panel->GetViewByID(SidePanelWebUIView::kSidePanelWebViewId);
    if (!ai_chat_side_panel) {
      return nullptr;
    }
    auto* side_panel_web_contents =
        (static_cast<views::WebView*>(ai_chat_side_panel))->web_contents();
    if (!side_panel_web_contents) {
      return nullptr;
    }
    content::WaitForLoadStop(side_panel_web_contents);

    auto* web_ui = side_panel_web_contents->GetWebUI();
    if (!web_ui) {
      return nullptr;
    }
    auto* ai_chat_ui = web_ui->GetController()->GetAs<AIChatUI>();
    if (!ai_chat_ui) {
      return nullptr;
    }
    return ai_chat_ui->GetPageHandlerForTesting();
  }

  void FetchPageContent(const base::Location& location,
                        ai_chat::AIChatTabHelper* helper,
                        std::string_view expected_text) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    base::RunLoop run_loop;
    helper->GetPageContent(
        base::BindLambdaForTesting(
            [&run_loop, expected_text](std::string text, bool is_video,
                                       std::string invalidation_token) {
              EXPECT_FALSE(is_video);
              EXPECT_EQ(text, expected_text);
              run_loop.Quit();
            }),
        "");
    run_loop.Run();
  }

 protected:
  net::test_server::EmbeddedTestServer https_server_;

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
};

IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest, PrintPreview) {
  browser()->window()->SetContentsSize(gfx::Size(800, 600));

  auto* chat_tab_helper =
      ai_chat::AIChatTabHelper::FromWebContents(GetActiveWebContents());
  ASSERT_TRUE(chat_tab_helper);
  chat_tab_helper->SetUserOptedIn(true);
  auto* ai_chat_page_handler = OpenAIChatSidePanel();
  ASSERT_TRUE(ai_chat_page_handler);

  NavigateURL(https_server_.GetURL("docs.google.com", "/long_canvas.html"));
  CreatePrintPreview(ai_chat_page_handler);
#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
  FetchPageContent(
      FROM_HERE, chat_tab_helper,
      "This is the way.\n\nI have spoken.\nWherever I Go, He Goes.");
  // Panel is still active so we don't need to set it up again

  // Page recognition host with a canvas element
  NavigateURL(https_server_.GetURL("docs.google.com", "/canvas.html"));
  CreatePrintPreview(ai_chat_page_handler);
  FetchPageContent(FROM_HERE, chat_tab_helper, "this is the way");
#if BUILDFLAG(IS_WIN)
  // Unsupported locale should return no content for Windows only
  // Other platforms do not use locale for extraction
  const brave_l10n::test::ScopedDefaultLocale locale("xx_XX");
  NavigateURL(https_server_.GetURL("docs.google.com", "/canvas.html"));
  CreatePrintPreview(ai_chat_page_handler);
  FetchPageContent(FROM_HERE, chat_tab_helper, "");
#endif  // #if BUILDFLAG(IS_WIN)
#else
  FetchPageContent(FROM_HERE, chat_tab_helper, "");
#endif

  // Not supported on other hosts
  NavigateURL(https_server_.GetURL("a.com", "/long_canvas.html"));
  CreatePrintPreview(ai_chat_page_handler);
  FetchPageContent(FROM_HERE, chat_tab_helper, "");
}

#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest, PrintPreviewPagesLimit) {
  browser()->window()->SetContentsSize(gfx::Size(800, 600));

  auto* chat_tab_helper =
      ai_chat::AIChatTabHelper::FromWebContents(GetActiveWebContents());
  ASSERT_TRUE(chat_tab_helper);
  chat_tab_helper->SetUserOptedIn(true);
  auto* ai_chat_page_handler = OpenAIChatSidePanel();
  ASSERT_TRUE(ai_chat_page_handler);

  NavigateURL(
      https_server_.GetURL("docs.google.com", "/extra_long_canvas.html"));
  CreatePrintPreview(ai_chat_page_handler);
  std::string expected_string(19, '\n');
  base::StrAppend(&expected_string, {"This is the way."});
  FetchPageContent(FROM_HERE, chat_tab_helper, expected_string);
}

IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest, PrintPreviewContextLimit) {
  browser()->window()->SetContentsSize(gfx::Size(800, 600));

  auto* chat_tab_helper =
      ai_chat::AIChatTabHelper::FromWebContents(GetActiveWebContents());
  ASSERT_TRUE(chat_tab_helper);
  chat_tab_helper->SetUserOptedIn(true);
  auto* ai_chat_page_handler = OpenAIChatSidePanel();
  ASSERT_TRUE(ai_chat_page_handler);

  chat_tab_helper->SetMaxContentLengthForTesting(10);
  NavigateURL(https_server_.GetURL("docs.google.com", "/long_canvas.html"));
  CreatePrintPreview(ai_chat_page_handler);
  FetchPageContent(FROM_HERE, chat_tab_helper, "This is the way.");

  chat_tab_helper->SetMaxContentLengthForTesting(20);
  NavigateURL(https_server_.GetURL("docs.google.com", "/long_canvas.html"));
  CreatePrintPreview(ai_chat_page_handler);
  FetchPageContent(FROM_HERE, chat_tab_helper,
                   "This is the way.\n\nI have spoken.");
}
#endif
