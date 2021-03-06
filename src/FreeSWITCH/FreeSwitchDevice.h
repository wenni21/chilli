#pragma once
#include "../Device/Device.h"
#include <log4cplus/logger.h>


namespace chilli{
namespace FreeSwitch {

	class FreeSwitchModule;
	class FreeSwitchDevice :public Device::Device {
	public:
		FreeSwitchDevice(FreeSwitchModule * model, const std::string &ext, const std::string &smFileName);
		virtual ~FreeSwitchDevice();

		//inherit from SendInterface
		virtual void fireSend(const fsm::FireDataType & fireData, const void * param) override;

	protected:
		void processSend(const fsm::FireDataType & fireData, const void * param, bool & bHandled);
	private:
		FreeSwitchModule * m_model;

	};
	typedef std::shared_ptr<FreeSwitchDevice>  FreeSwitchDevicePtr;
}
}