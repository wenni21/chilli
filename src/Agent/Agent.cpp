#include "Agent.h"
#include <log4cplus/loggingmacros.h>
#include <json/config.h>
#include <json/json.h>
#include "../model/ProcessModule.h"

namespace chilli{
namespace Agent{


Agent::Agent(model::ProcessModule * model, const std::string &ext, const std::string &smFileName)
	:PerformElement(model, ext), m_SMFileName(smFileName)
{
	std::string logName= "Agent";
	log = log4cplus::Logger::getInstance(logName);
	LOG4CPLUS_DEBUG(log, "." + this->getId(), " new a Agent object.");
}

Agent::~Agent(){
	LOG4CPLUS_DEBUG(log, "." + this->getId(), " destruction a Agent object.");
}

void Agent::processSend(const fsm::FireDataType &fireData, const void * param, bool & bHandled)
{

}

void Agent::fireSend(const fsm::FireDataType &fireData,const void * param)
{
	LOG4CPLUS_TRACE(log, "." + this->getId(), " fireSend:" << fireData.event);
	bool bHandled = false;
	processSend(fireData, param, bHandled);
}

void Agent::Start()
{
	LOG4CPLUS_INFO(log, "." + this->getId(), " Start.");
	for (auto & it : m_StateMachines) {
		it.second->start(false);
	}
}

void Agent::Stop()
{
	for (auto & it : m_StateMachines) {
		it.second->stop();
	}
	LOG4CPLUS_INFO(log, "." + this->getId(), " Stop.");
}

bool Agent::IsClosed()
{
	return m_StateMachines.empty();
}

bool Agent::pushEvent(const model::EventType_t & Event)
{
	return m_EvtBuffer.Put(Event);
}

void Agent::mainEventLoop()
{
	try
	{

		model::EventType_t Event;
		if (m_EvtBuffer.Get(Event, 0) && !Event->eventName.empty()) {
			const Json::Value & jsonEvent = Event->jsonEvent;

			std::string connectid;

			fsm::TriggerEvent evt(Event->eventName, Event->type);

			for (auto & it : jsonEvent.getMemberNames()) {
				evt.addVars(it, jsonEvent[it]);
			}

			LOG4CPLUS_DEBUG(log, "." + this->getId(), " Recived a event," << Event->origData);

			if (m_StateMachines.begin() == m_StateMachines.end()) {
				StateMachine sm(fsm::fsmParseFile(m_SMFileName));

				if (sm == nullptr){
					LOG4CPLUS_ERROR(log, "." + getId(), m_SMFileName << " parse filed.");
					return;
				}

				sm->setLoggerId(this->log.getName());
				sm->setSessionID(m_Id);
				sm->setOnTimer(this->m_model);
				m_StateMachines[connectid] = sm;

				for (auto & itt : this->m_Vars.getMemberNames())
				{
					sm->setVar(itt, this->m_Vars[itt]);
				}

				for (auto & itt : model::ProcessModule::g_Modules) {
					sm->addSendImplement(itt.get());
				}

				sm->addSendImplement(this);
				sm->start(false);
			}

			const auto & it = m_StateMachines.begin();
			it->second->pushEvent(evt);
			it->second->mainEventLoop();

			if (it->second->isInFinalState()) {
				it->second->stop();
				m_StateMachines.erase(it);
			}
		}
	}
	catch (std::exception & e)
	{
		LOG4CPLUS_ERROR(log, "." + this->getId(), e.what());
	}
}

}
}