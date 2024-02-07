#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class WalletPageHandler : public brave_wallet::mojom::PageHandler {
public:
  WalletPageHandler(
      mojo::PendingRemote<brave_wallet::mojom::Page> page,
      mojo::PendingReceiver<brave_wallet::mojom::PageHandler> receiver);

  WalletPageHandler(const WalletPageHandler&) = delete;
  WalletPageHandler& operator=(const WalletPageHandler&) = delete;
  ~WalletPageHandler() override;


  // brave_wallet::mojom::PageHandler
  void ShowApprovePanelUI() override;

 private:
  mojo::PendingRemote<brave_wallet::mojom::Page> page_;
  mojo::Receiver<brave_wallet::mojom::PageHandler> receiver_;
  base::WeakPtrFactory<WalletPageHandler> weak_ptr_factory_{this};
};
