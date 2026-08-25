#include "../include/SnaccROSEInterfaces.h"
#include "snacc-assert.h"

void SnaccROSESender::ClearAllSubscriptions()
{
	ASSERT_FAILED("ClearAllSubscriptions not implemented on this SnaccROSESender");
}

void SnaccROSESender::ClearSubscribedEvents(int moduleIid)
{
	(void)moduleIid;
	ASSERT_FAILED("ClearSubscribedEvents not implemented on this SnaccROSESender");
}

void SnaccROSESender::ClearSupportedInvokes(int moduleIid)
{
	(void)moduleIid;
	ASSERT_FAILED("ClearSupportedInvokes not implemented on this SnaccROSESender");
}

void SnaccROSESender::SetSubscribedEvents(int moduleIid, const std::list<int>& eventOpIds)
{
	(void)moduleIid;
	(void)eventOpIds;
	ASSERT_FAILED("SetSubscribedEvents not implemented on this SnaccROSESender");
}

void SnaccROSESender::AddSubscribedEvent(int moduleIid, unsigned int uiEventOpId)
{
	(void)moduleIid;
	(void)uiEventOpId;
	ASSERT_FAILED("AddSubscribedEvent not implemented on this SnaccROSESender");
}

void SnaccROSESender::AddSupportedInvoke(int moduleIid, unsigned int uiInvokeOpId)
{
	(void)moduleIid;
	(void)uiInvokeOpId;
	ASSERT_FAILED("AddSupportedInvoke not implemented on this SnaccROSESender");
}

void SnaccROSESender::SetSupportedInvokes(int moduleIid, const std::list<int>& invokeOpIds)
{
	(void)moduleIid;
	(void)invokeOpIds;
	ASSERT_FAILED("SetSupportedInvokes not implemented on this SnaccROSESender");
}

bool SnaccROSESender::IsSubscribedEvent(unsigned int uiEventOpId) const
{
	(void)uiEventOpId;
	ASSERT_FAILED("IsSubscribedEvent not implemented on this SnaccROSESender");
	return false;
}

bool SnaccROSESender::IsSupportedInvoke(unsigned int uiInvokeOpId) const
{
	(void)uiInvokeOpId;
	ASSERT_FAILED("IsSupportedInvoke not implemented on this SnaccROSESender");
	return false;
}

bool SnaccROSEComponent::IsSubscribedEvent(unsigned int uiEventOpId) const
{
	ASSERT(m_pSB, "SnaccROSEComponent has no SnaccROSESender");
	return m_pSB ? m_pSB->IsSubscribedEvent(uiEventOpId) : false;
}

bool SnaccROSEComponent::IsSupportedInvoke(unsigned int uiInvokeOpId) const
{
	ASSERT(m_pSB, "SnaccROSEComponent has no SnaccROSESender");
	return m_pSB ? m_pSB->IsSupportedInvoke(uiInvokeOpId) : false;
}

void SnaccROSEComponent::ClearAllSubscriptions() const
{
	ASSERT(m_pSB, "SnaccROSEComponent has no SnaccROSESender");
	if (m_pSB)
		m_pSB->ClearAllSubscriptions();
}

void SnaccROSEComponent::ClearSubscribedEvents(int moduleIid) const
{
	ASSERT(m_pSB, "SnaccROSEComponent has no SnaccROSESender");
	if (m_pSB)
		m_pSB->ClearSubscribedEvents(moduleIid);
}

void SnaccROSEComponent::ClearSupportedInvokes(int moduleIid) const
{
	ASSERT(m_pSB, "SnaccROSEComponent has no SnaccROSESender");
	if (m_pSB)
		m_pSB->ClearSupportedInvokes(moduleIid);
}

void SnaccROSEComponent::SetSubscribedEvents(int moduleIid, const std::list<int>& eventOpIds) const
{
	ASSERT(m_pSB, "SnaccROSEComponent has no SnaccROSESender");
	if (m_pSB)
		m_pSB->SetSubscribedEvents(moduleIid, eventOpIds);
}

void SnaccROSEComponent::AddSubscribedEvent(int moduleIid, unsigned int uiEventOpId) const
{
	ASSERT(m_pSB, "SnaccROSEComponent has no SnaccROSESender");
	if (m_pSB)
		m_pSB->AddSubscribedEvent(moduleIid, uiEventOpId);
}

void SnaccROSEComponent::AddSupportedInvoke(int moduleIid, unsigned int uiInvokeOpId) const
{
	ASSERT(m_pSB, "SnaccROSEComponent has no SnaccROSESender");
	if (m_pSB)
		m_pSB->AddSupportedInvoke(moduleIid, uiInvokeOpId);
}

void SnaccROSEComponent::SetSupportedInvokes(int moduleIid, const std::list<int>& invokeOpIds) const
{
	ASSERT(m_pSB, "SnaccROSEComponent has no SnaccROSESender");
	if (m_pSB)
		m_pSB->SetSupportedInvokes(moduleIid, invokeOpIds);
}
