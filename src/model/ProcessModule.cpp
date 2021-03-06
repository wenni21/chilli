#include "ProcessModule.h"
#include <log4cplus/loggingmacros.h>
#include <regex>
#include <sys/stat.h>

namespace chilli{
namespace model{
	std::vector<ProcessModulePtr> ProcessModule::g_Modules;
	std::recursive_mutex ProcessModule::g_PEMtx;
	model::PerformElementMap ProcessModule::g_PerformElements;

	ProcessModule::ProcessModule(const std::string & modelId, uint32_t threadSize) :SendInterface(modelId, this),
		m_bRunning(false), m_executeThread(threadSize), m_Id(modelId)
	{
	}

	ProcessModule::~ProcessModule()
	{
		if (m_bRunning) {
			Stop();
		}

		for (auto it = m_PerformElements.begin(); it != m_PerformElements.end();) {
			removePerfromElement(it++->first);
		}

		for (const auto & it : m_existStateMachine){
			delete it.second.sm;
		}

	}

	int ProcessModule::Start()
	{
		if (!m_bRunning) {
			m_bRunning = true;
			m_thread = std::thread(&ProcessModule::_run, this);
			for (auto & it : m_executeThread) {
				it.th = std::thread(&ProcessModule::_execute, this, &it.eventQueue);
			}
		}
		else {
			LOG4CPLUS_WARN(log, "." + this->getId(), " already running for this module.");
		}
		return 0;
	}

	int ProcessModule::Stop()
	{
		if (m_bRunning) {

			for (auto & it : m_PerformElements) {
				it.second->Stop();
			}

			m_bRunning = false;
			chilli::model::EventType_t stopEvent(new model::_EventType(Json::nullValue));
			this->m_RecEvtBuffer.Put(stopEvent);

			if (m_thread.joinable()) {
				m_thread.join();
			}

			for (auto & it : m_executeThread) {
				if (it.th.joinable())
					it.th.join();
			}
		}
		return 0;
	}

	/*
	PerformElementMap  ProcessModule::GetExtensions()
	{
		// TODO: insert return statement here
		std::unique_lock<std::recursive_mutex> lck(g_PEMtx);
		return g_PerformElements;
	}
	*/

	void ProcessModule::PushEvent(const EventType_t & Event)
	{
		this->m_RecEvtBuffer.Put(Event);
	}

	void ProcessModule::_run()
	{
		return this->run();
	}

	void ProcessModule::_execute(helper::CEventBuffer<model::EventType_t> * eventQueue)
	{
		return this->execute(eventQueue);
	}
	
	void ProcessModule::OnTimer(unsigned long timerId, const std::string & attr, void * userdata)
	{
		Json::Value jsonEvent;
		Json::CharReaderBuilder b;
		std::shared_ptr<Json::CharReader> jsonReader(b.newCharReader());
		std::string jsonerr;
		std::string peId;

		if (jsonReader->parse(attr.c_str(),attr.c_str()+attr.length(), &jsonEvent, &jsonerr)) {
			jsonEvent["id"] = jsonEvent["sessionId"];

			if (jsonEvent["id"].isString()) {
				peId = jsonEvent["id"].asString();
			}
		}
		chilli::model::EventType_t evt(new model::_EventType(jsonEvent));
		m_RecEvtBuffer.Put(evt);
	}

	const log4cplus::Logger & ProcessModule::getLogger()
	{
		return this->log;
	}

	const std::string ProcessModule::getId()
	{
		return m_Id;
	}

	fsm::StateMachine * ProcessModule::createStateMachine(const std::string & filename)
	{
		struct stat fileStatus;
		stat(filename.c_str(), &fileStatus);
		StateMachineModifyTime smmt;

		std::unique_lock<std::mutex>lck(m_m_existStateMachineMtx);
		const auto & it = m_existStateMachine.find(filename);
		if (it != m_existStateMachine.end()){
			smmt = it->second;
		}

		if (smmt.modifytime != fileStatus.st_mtime) {
			delete smmt.sm;
			smmt.sm = fsm::fsmParseFile(filename);
			smmt.modifytime = fileStatus.st_mtime;
			m_existStateMachine[filename] = smmt;
		}
		return  new fsm::StateMachine(*smmt.sm);
	}

	bool ProcessModule::addPerformElement(const std::string &peId, PerformElementPtr & peptr)
	{
		std::unique_lock<std::recursive_mutex> lck(g_PEMtx);

		if (this->g_PerformElements.find(peId) == this->g_PerformElements.end()){
			this->g_PerformElements[peId] = peptr;
		}
		if (this->m_PerformElements.find(peId) == this->m_PerformElements.end()) {
			this->m_PerformElements[peId] = peptr;
			return true;
		}

		return false;
	}

	PerformElementPtr ProcessModule::removePerfromElement(const std::string & peId)
	{
		std::unique_lock<std::recursive_mutex> lck(g_PEMtx);
		PerformElementPtr peptr;
		const auto & it = this->m_PerformElements.find(peId);
		if (it != this->m_PerformElements.end())
			peptr = it->second;

		this->g_PerformElements.erase(peId);
		this->m_PerformElements.erase(peId);

		return peptr;
	}

	chilli::model::PerformElementPtr ProcessModule::getPerformElement(const std::string & peId)
	{
		std::unique_lock<std::recursive_mutex> lck(g_PEMtx);
		const auto & it = this->m_PerformElements.find(peId);
		if (it != this->m_PerformElements.end()) {
			return it->second;
		}

		return nullptr;
	}

	chilli::model::PerformElementPtr ProcessModule::getPerformElementByGlobal(const std::string & peId)
	{
		std::unique_lock<std::recursive_mutex> lck(g_PEMtx);

		do 
		{
			const auto & it = this->m_PerformElements.find(peId);
			if (it != this->m_PerformElements.end()) {
				return it->second;
			}
		} while (0);

		do 
		{
			const auto & it = this->g_PerformElements.find(peId);
			if (it != this->g_PerformElements.end()) {
				return it->second;
			}
		} while (0);
		return nullptr;
	}

	uint64_t ProcessModule::getPerformElementCount()
	{
		std::unique_lock<std::recursive_mutex> lck(g_PEMtx);
		return this->m_PerformElements.size();
	}

}
}