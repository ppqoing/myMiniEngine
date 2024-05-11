#include"config_manager.h"
configManager::configManager()
{
	configStruct temp;
	ifstream f(CONFIG_FILE_PATH);
	cereal::JSONInputArchive conf_a(f);
	conf_a(cereal::make_nvp("config", temp));
}

configManager::~configManager()
{
	ofstream of(CONFIG_FILE_PATH);
	cereal::JSONOutputArchive conf_o(of);
	conf_o(cereal::make_nvp("config", config));
}

configStruct configManager::GetConfig()
{
	return config;
}
