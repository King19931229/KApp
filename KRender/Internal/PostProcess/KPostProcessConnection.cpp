#include "KPostProcessConnection.h"
#include "KPostProcessPass.h"
#include "KPostProcessTexture.h"
#include "KPostProcessManager.h"
#include "KBase/Publish/KStringParser.h"
#include "Internal/KRenderGlobal.h"

#include <assert.h>

KPostProcessConnection::KPostProcessConnection(KPostProcessManager* mgr)
	: m_Mgr(mgr)
{
	// TODO GUID
	char szTempBuffer[256] = { 0 };
	size_t address = (size_t)this;
	ASSERT_RESULT(KStringParser::ParseFromSIZE_T(szTempBuffer, ARRAY_SIZE(szTempBuffer), &address, 1));
	m_ID = szTempBuffer;
}

KPostProcessConnection::KPostProcessConnection(KPostProcessManager* mgr, IDType id)
	: m_Mgr(mgr),
	m_ID(id)
{
}

KPostProcessConnection::~KPostProcessConnection()
{
}

const char* KPostProcessConnection::msIDKey = "id";
const char* KPostProcessConnection::msInKey = "in";
const char* KPostProcessConnection::msOutKey = "out";

bool KPostProcessConnection::Save(IKJsonDocumentPtr jsonDoc, IKJsonValuePtr& object)
{
	object = jsonDoc->CreateObject();

	object->AddMember(msIDKey, jsonDoc->CreateString(m_ID.c_str()));

	auto in = jsonDoc->CreateObject();
	in->AddMember(KPostProcessData::msIDKey, jsonDoc->CreateString(m_Input.node->ID().c_str()));
	in->AddMember(KPostProcessData::msSlotKey, jsonDoc->CreateInt(m_Input.slot));

	object->AddMember(msInKey, in);

	auto out = jsonDoc->CreateObject();
	PostProcessNodeType outType = m_Output.node->GetType();

	out->AddMember(KPostProcessData::msIDKey, jsonDoc->CreateString(m_Output.node->ID().c_str()));
	out->AddMember(KPostProcessData::msSlotKey, jsonDoc->CreateInt(m_Output.slot));

	object->AddMember(msOutKey, out);

	return true;
}

bool KPostProcessConnection::Load(IKJsonValuePtr& object)
{
	m_ID = object->GetMember(msIDKey)->GetString();

	{
		IKPostProcessNode* inNode = nullptr;
		int16_t inSlot = INVALID_SLOT_INDEX;

		auto in_json = object->GetMember(msInKey);

		auto in_pass_id = in_json->GetMember(KPostProcessData::msIDKey)->GetString();
		inNode = m_Mgr->GetNode(in_pass_id);
		assert(inNode);

		inSlot = (int16_t)in_json->GetMember(KPostProcessData::msSlotKey)->GetInt();
		assert(inSlot >= 0);

		m_Input.Init(inNode, inSlot);
		ASSERT_RESULT(inNode->AddInputConnection(this, inSlot));
	}

	{
		IKPostProcessNode* outNode = nullptr;
		int16_t outSlot = INVALID_SLOT_INDEX;

		auto out_json = object->GetMember(msOutKey);

		auto out_pass_id = out_json->GetMember(KPostProcessData::msIDKey)->GetString();
		outNode = m_Mgr->GetNode(out_pass_id);
		assert(outNode);

		outSlot = (int16_t)out_json->GetMember(KPostProcessData::msSlotKey)->GetInt();
		assert(outSlot >= 0);

		m_Output.Init(outNode, outSlot);
		ASSERT_RESULT(outNode->AddOutputConnection(this, outSlot));
	}

	return true;
}