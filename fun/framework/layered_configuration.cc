#include "fun/framework/layered_configuration.h"
#include "fun/base/container/set.h"
#include "fun/base/exception.h"

namespace fun {
namespace framework {

LayeredConfiguration::LayeredConfiguration() {}

LayeredConfiguration::~LayeredConfiguration() {}

void LayeredConfiguration::Add(ConfigurationBase::Ptr config) {
  Add(config, Highest(), false);
}

void LayeredConfiguration::Add(ConfigurationBase::Ptr config,
                               const String& label) {
  Add(config, label, Highest(), false);
}

void LayeredConfiguration::Add(ConfigurationBase::Ptr config, int32 priority) {
  Add(config, priority, false);
}

void LayeredConfiguration::Add(ConfigurationBase::Ptr config,
                               const String& label, int32 priority) {
  Add(config, label, priority, false);
}

void LayeredConfiguration::AddWriteable(ConfigurationBase::Ptr config,
                                        int32 priority) {
  Add(config, priority, true);
}

void LayeredConfiguration::Add(ConfigurationBase::Ptr config, int32 priority,
                               bool writeable) {
  Add(config, String(), priority, writeable);
}

void LayeredConfiguration::Add(ConfigurationBase::Ptr config,
                               const String& label, int32 priority,
                               bool writeable) {
  ConfigItem item;
  item.config = config;
  item.priority = priority;
  item.writeable = writeable;
  item.label = label;

  ConfigList::iterator it = configs_.begin();
  // prioritized...
  while (it != configs_.end() && it->priority < priority) ++it;
  configs_.Insert(it, item);
}

void LayeredConfiguration::RemoveConfiguration(ConfigurationBase::Ptr config) {
  for (ConfigList::iterator it = configs_.begin(); it != configs_.end(); ++it) {
    if (it->config == config) {
      configs_.erase(it);
      break;
    }
  }
}

ConfigurationBase::Ptr LayeredConfiguration::Find(const String& label) const {
  for (ConfigList::const_iterator it = configs_.begin(); it != configs_.end();
       ++it) {
    if (it->label == label) {
      return it->config;
    }
  }
  return nullptr;
}

bool LayeredConfiguration::GetRaw(const String& key, String& value) const {
  for (ConfigList::const_iterator it = configs_.begin(); it != configs_.end();
       ++it) {
    if (it->config->GetRaw(key, value)) {
      return true;
    }
  }
  return false;
}

void LayeredConfiguration::SetRaw(const String& key, const String& value) {
  for (ConfigList::iterator it = configs_.begin(); it != configs_.end(); ++it) {
    if (it->writeable) {
      it->config->SetRaw(key, value);
      return;
    }
  }
  throw RuntimeException(
      "No writeable configuration object to store the property", key);
}

void LayeredConfiguration::Enumerate(const String& key,
                                     Array<String>& range) const {
  std::set<String> key_set;
  for (ConfigList::const_iterator itc = configs_.begin(); itc != configs_.end();
       ++itc) {
    Keys part_range;
    itc->config->Enumerate(key, part_range);
    for (Keys::const_iterator itr = part_range.begin(); itr != part_range.end();
         ++itr) {
      if (key_set.Find(*itr) == key_set.end()) {
        range.PushBack(*itr);
        key_set.Insert(*itr);
      }
    }
  }
}

void LayeredConfiguration::RemoveRaw(const String& key) {
  for (ConfigList::iterator it = configs_.begin(); it != configs_.end(); ++it) {
    if (it->writeable) {
      it->config->Remove(key);
      return;
    }
  }
}

int32 LayeredConfiguration::Lowest() const {
  if (configs_.IsEmpty()) {
    return 0;
  } else {
    return configs_.Front().priority - 1;
  }
}

int32 LayeredConfiguration::Highest() const {
  if (configs_.IsEmpty()) {
    return 0;
  } else {
    return configs_.Back().priority + 1;
  }
}

}  // namespace framework
}  // namespace fun
