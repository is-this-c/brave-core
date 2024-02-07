#include "base/memory/raw_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

class ChromeBrowserState;

namespace brave_wallet {

class KeyringService;

class WalletHandler : public mojom::WalletHandler {
 public:
  WalletHandler(mojo::PendingReceiver<mojom::WalletHandler> receiver,
                ChromeBrowserState* browser_state);

  WalletHandler(const WalletHandler&) = delete;
  WalletHandler& operator=(const WalletHandler&) = delete;
  ~WalletHandler() override;

  // mojom::WalletHandler:
  void GetWalletInfo(GetWalletInfoCallback) override;

 private:
  mojo::Receiver<mojom::WalletHandler> receiver_{this};
  const raw_ptr<KeyringService> keyring_service_ = nullptr;
};

}  // namespace brave_wallet
