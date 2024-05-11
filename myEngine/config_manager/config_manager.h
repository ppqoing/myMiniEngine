#pragma once
#include"../Sigleton.h"
#include"../cereal.h"
#include"../baseUtil.h"
#define CONFIG_FILE_PATH "baseFile/config.json"
struct configStruct
{
	bool      mFullscreenState = false;// fullscreen enabled
	// Set true to use 4X MSAA (?.1.8).  The default is false.
	bool      m4xMsaaState = false;    // 4X MSAA enabled
	UINT    m4xMsaaQuality = 0;      // quality level of 4X MSAA
	int mClientWidth = 800;
	int mClientHeight = 600;
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(cereal::make_nvp("FullscreenState", mFullscreenState),
			cereal::make_nvp("4xMsaaState", m4xMsaaState),
			cereal::make_nvp("4xMsaaQuality", m4xMsaaQuality),
			cereal::make_nvp("ClientWidth", mClientWidth),
			cereal::make_nvp("ClientHeight", mClientHeight)
		);
	}
};
class configManager :public Sigleton<configManager>
{
	friend class Sigleton<configManager>;
public:
	configManager();
	~configManager();
	configStruct GetConfig();
private:
	configStruct config;
};

