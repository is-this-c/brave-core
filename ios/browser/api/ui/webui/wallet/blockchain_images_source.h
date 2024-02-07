#include <string>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "ios/web/public/webui/url_data_source_ios.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

// This serves background image data.
class BlockchainImagesSource : public web::URLDataSourceIOS {
 public:
  explicit BlockchainImagesSource(const base::FilePath& base_path);

  ~BlockchainImagesSource() override;

  BlockchainImagesSource(const BlockchainImagesSource&) = delete;
  BlockchainImagesSource& operator=(const BlockchainImagesSource&) = delete;
  std::string getImagePath();

 private:
  friend class BlockchainImagesSourceTest;

  // web::URLDataSourceIOS overrides:
  std::string GetSource() const override;
  void StartDataRequest(const std::string& path,
                        GotDataCallback callback) override;
  std::string GetMimeType(const std::string& path) const override;
  bool AllowCaching() const override;

  void OnGotImageFile(GotDataCallback callback,
                      absl::optional<std::string> input);

  base::FilePath base_path_;
  base::WeakPtrFactory<BlockchainImagesSource> weak_factory_;
};

}  // namespace brave_wallet
