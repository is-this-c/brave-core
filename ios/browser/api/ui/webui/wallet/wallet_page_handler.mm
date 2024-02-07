#include "brave/ios/browser/api/ui/webui/wallet/wallet_page_handler.h"

#include <utility>

WalletPageHandler::WalletPageHandler(mojo::PendingRemote<brave_wallet::mojom::Page> page,
                                                     mojo::PendingReceiver<brave_wallet::mojom::PageHandler> receiver)
    : page_(std::move(page)), receiver_(this, std::move(receiver)) {

}

WalletPageHandler::~WalletPageHandler() = default;

void WalletPageHandler::ShowApprovePanelUI() {
  
}
