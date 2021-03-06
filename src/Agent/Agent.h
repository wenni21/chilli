#pragma once
#include "../model/PerformElement.h"
#include "../CEventBuffer.h"


namespace chilli{
namespace Agent{

class Agent :public model::PerformElement{
protected:
	typedef std::shared_ptr<fsm::StateMachine> StateMachine;

public:
	Agent(model::ProcessModule * model, const std::string &ext, const std::string &smFileName);
	virtual ~Agent();

public:
	//inherit from PerformElement
	virtual void Start() override;
	virtual void Stop() override;
	virtual bool IsClosed() override;
	virtual bool pushEvent(const model::EventType_t &evt) override;
	virtual void mainEventLoop() override;

	//inherit from SendInterface
	virtual void fireSend(const fsm::FireDataType &fireData, const void * param) override;

protected:
	void processSend(const fsm::FireDataType &fireData, const void * param, bool & bHandled);

	//private:
	std::map<std::string, StateMachine> m_StateMachines;
	const std::string m_SMFileName;
	helper::CEventBuffer<model::EventType_t> m_EvtBuffer;
};
typedef std::shared_ptr<Agent>  AgentPtr;
}
}