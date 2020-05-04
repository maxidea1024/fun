#include "fun/framework/logging_configurator.h"

#include "fun/base/logging/formatting_sink.h"
#include "fun/base/logging/logger.h"
#include "fun/base/logging/logging_factory.h"
#include "fun/base/logging/logging_registry.h"
#include "fun/base/logging/pattern_formatter.h"

namespace fun {
namespace framework {

LoggingConfigurator::LoggingConfigurator() {}

LoggingConfigurator::~LoggingConfigurator() {}

void LoggingConfigurator::Configure(ConfigurationBase::Ptr config) {
  fun_check_ptr(config);

  {
    ConfigurationBase::Ptr formatters_config(
        config->CreateView("logging.formatters"));
    ConfigureFormatters(formatters_config);
  }

  {
    ConfigurationBase::Ptr sinks_config(config->CreateView("logging.sinks"));
    ConfigureSinks(sinks_config);
  }

  {
    ConfigurationBase::Ptr loggers_config(
        config->CreateView("logging.loggers"));
    ConfigureLoggers(loggers_config);
  }
}

void LoggingConfigurator::ConfigureFormatters(ConfigurationBase::Ptr config) {
  ConfigurationBase::Keys formatter_names;
  config->GetKeys(formatter_names);

  for (const auto& formatter_name : formatter_names) {
    ConfigurationBase::Ptr formatter_config(config->CreateView(formatter_name));
    LogFormatter::Ptr formatter(CreateFormatter(formatter_config));
    LoggingRegistry::DefaultRegistry().RegisterFormatter(formatter_name,
                                                         formatter);
  }
}

void LoggingConfigurator::ConfigureSinks(ConfigurationBase::Ptr config) {
  ConfigurationBase::Keys sink_names;
  config->GetKeys(sink_names);

  for (const auto& sink_name : sink_names) {
    ConfigurationBase::Ptr sink_config(config->CreateView(sink_name));
    LogSink::Ptr sink(CreateSink(sink_config));
    LoggingRegistry::DefaultRegistry().RegisterSink(sink_name, sink);
  }

  for (const auto& sink_name : sink_names) {
    ConfigurationBase::Ptr sink_config(config->CreateView(sink_name));
    LogSink::Ptr sink(
        LoggingRegistry::DefaultRegistry().SinkForName(sink_name));
    ConfigureSink(sink, sink_config);
  }
}

void LoggingConfigurator::ConfigureLoggers(ConfigurationBase::Ptr config) {
  ConfigurationBase::Keys logger_classes;
  config->GetKeys(logger_classes);

  Map<String, ConfigurationBase::Ptr> logger_map;
  for (const auto& logger_class : logger_classes) {
    ConfigurationBase::Ptr logger_config(config->CreateView(logger_class));
    logger_map.Add(logger_config->GetString("name", ""), logger_config);
  }

  for (auto& pair : logger_map) {
    ConfigureLogger(pair.value);
  }
}

LogFormatter::Ptr LoggingConfigurator::CreateFormatter(
    ConfigurationBase::Ptr config) {
  LogFormatter::Ptr formatter(LoggingFactory::DefaultFactory().CreateFormatter(
      config->GetString("class")));

  ConfigurationBase::Keys props;
  config->GetKeys(props);

  for (const auto& prop : props) {
    if (prop != "class") {
      formatter->SetProperty(prop, config->GetString(prop));
    }
  }

  return formatter;
}

LogSink::Ptr LoggingConfigurator::CreateSink(ConfigurationBase::Ptr config) {
  LogSink::Ptr sink(
      LoggingFactory::DefaultFactory().CreateSink(config->GetString("class")));
  LogSink::Ptr wrapper(sink);

  ConfigurationBase::Keys props;
  config->GetKeys(props);

  for (const auto& prop : props) {
    if (prop == "pattern") {
      LogFormatter::Ptr pattern_formatter(
          new PatternFormatter(config->GetString(prop)));
      wrapper = new FormattingSink(pattern_formatter, sink);
    } else if (prop == "formatter") {
      FormattingSink::Ptr formatting_sink(new FormattingSink(0, sink));
      if (config->HasProperty("formatter.class")) {
        ConfigurationBase::Ptr formatter_config(config->CreateView(prop));
        LogFormatter::Ptr formatter(CreateFormatter(formatter_config));
        formatting_sink->SetFormatter(formatter);
      } else {
        formatting_sink->SetProperty(prop, config->GetString(prop));
      }

      wrapper = formatting_sink;
    }
  }

  return wrapper;
}

void LoggingConfigurator::ConfigureSink(LogSink::Ptr sink,
                                        ConfigurationBase::Ptr config) {
  ConfigurationBase::Keys prop_list;
  config->GetKeys(prop_list);

  for (const auto& prop : prop_list) {
    if (prop == "pattern" && prop != "formatter" && prop != "class") {
      sink->SetProperty(prop, config->GetString(prop));
    }
  }
}

void LoggingConfigurator::ConfigureLogger(ConfigurationBase::Ptr config) {
  Logger& logger = Logger::Get(config->GetString("name", ""));

  ConfigurationBase::Keys prop_list;
  config->GetKeys(prop_list);

  for (const auto& prop : prop_list) {
    if (prop == "sink" && config->HasProperty("sink.class")) {
      ConfigurationBase::Ptr sink_config(config->CreateView(prop));
      LogSink::Ptr sink(CreateSink(sink_config));
      ConfigureSink(sink, sink_config);
      Logger::SetSink(logger.GetName(), sink);
    } else if (prop != "name") {
      Logger::SetProperty(logger.GetName(), prop, config->GetString(prop));
    }
  }
}

}  // namespace framework
}  // namespace fun
