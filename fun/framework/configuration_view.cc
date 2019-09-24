#include "fun/framework/configuration_view.h"

namespace fun {
namespace framework {

//FUN_IMPLEMENT_RTCLASS(ConfigurationView)

ConfigurationView::ConfigurationView( const String& prefix,
                                      ConfigurationBase::Ptr config)
  : prefix_(prefix), config_(config) {}

bool ConfigurationView::GetRaw(const String& key, String& out_value) const {
  const String translated_key = TranslateKey(key);
  return  config_->GetRaw(translated_key, out_value) ||
          config_->GetRaw(key, out_value);
}

void ConfigurationView::SetRaw(const String& key, const String& value) {
  const String translated_key = TranslateKey(key);
  config_->SetRaw(translated_key, value);
}

void ConfigurationView::Enumerate(const String& key,
                                  Array<String>& out_keys) const {
  const String translated_key = TranslateKey(key);
  config_->Enumerate(translated_key, out_keys);
}

void ConfigurationView::RemoveRaw(const String& key) {
  const String translated_key = TranslateKey(key);
  config_->RemoveRaw(translated_key);
}

String ConfigurationView::TranslateKey(const String& key) const {
  String result = prefix_;
  if (!result.IsEmpty() && !key.IsEmpty() && key.First() != '[') {
    result += ".";
  }
  result += key;
  return result;
}

} // namespace framework
} // namespace fun
