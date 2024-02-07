#include "brave/ios/browser/api/ui/webui/wallet/wallet_common_ui.h"

#include <memory>

#include "base/version.h"
#include "brave/ios/browser/api/ui/webui/wallet/blockchain_images_source.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"
#include "brave/components/constants/webui_url_constants.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave_wallet {

void AddBlockchainTokenImageSource(ChromeBrowserState* browser_state) {
  base::FilePath path = browser_state->GetStatePath().DirName();
  path = path.AppendASCII(brave_wallet::kWalletBaseDirectory);
  web::URLDataSourceIOS::Add(
                             browser_state, new brave_wallet::BlockchainImagesSource(path));
}

bool IsBraveWalletOrigin(const url::Origin& origin) {
  return origin == url::Origin::Create(GURL(kBraveUIWalletPanelURL)) ||
  origin == url::Origin::Create(GURL(kBraveUIWalletPageURL));
}
}
