#include <stdint.h>

class ChromeBrowserState;
class Browser;

namespace url {
class Origin;
}  // namespace url

namespace brave_wallet {

void AddBlockchainTokenImageSource(ChromeBrowserState* browser_state);
bool IsBraveWalletOrigin(const url::Origin& origin);

}  // namespace brave_wallet
