#include "KPostProcessConnection.h"
#include "KPostProcessPass.h"
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
	in->AddMember(KPostProcessInputData::msPassKey, jsonDoc->CreateString(m_Input.pass->ID().c_str()));
	in->AddMember(KPostProcessInputData::msSlotKey, jsonDoc->CreateInt(m_Input.slot));
	object->AddMember(msInKey, in);

	auto out = jsonDoc->CreateObject();
	out->AddMember(KPostProcessOutputData::msTypeKey, jsonDoc->CreateInt(m_Output.type));
	switch (m_Output.type)
	{
		case POST_PROCESS_OUTPUT_PASS:
		{
			assert(m_Output.pass);
			out->AddMember(KPostProcessOutputData::msPassKey, jsonDoc->CreateString(m_Output.pass->ID().c_str()));
			break;
		}
		case POST_PROCESS_OUTPUT_TEXTURE:
		{
			assert(m_Output.texture);
			out->AddMember(KPostProcessOutputData::msTextureKey, jsonDoc->CreateString(m_Output.texture->GetPath()));
			break;
		}
		default:
		{
			assert(false && "impossible to reach");
			break;
		}
	}
	out->AddMember(KPostProcessOutputData::msSlotKey, jsonDoc->CreateInt(m_Output.slot));
	object->AddMember(msOutKey, out);

	return true;
}

bool KPostProcessConnection::Load(IKJsonValuePtr& object)
{
	m_ID = object->GetMember(msIDKey)->GetString();
	
	KPostProcessPass* inPass = nullptr;
	int16_t inSlot = INVALID_SLOT_INDEX;

	KPostProcessPass* outPass = nullptr;
	IKTexturePtr outTexture = nullptr;
	int16_t outSlot = INVALID_SLOT_INDEX;

	{
		auto in_json = object->GetMember(msInKey);

		auto in_pass_id = in_json->GetMember(KPostProcessInputData::msPassKey)->GetString();
		inPass = m_Mgr->GetPass(in_pass_id);
		assert(inPass);

		inSlot = (int16_t)in_json->GetMember(KPostProcessInputData::msSlotKey)->GetInt();
		assert(inSlot >= 0);

		m_Input.Init(inPass, inSlot);
	}

	{
		auto out_json = object->GetMember(msOutKey);

		outSlot = (int16_t)out_json->GetMember(KPostProcessOutputData::msSlotKey)->GetInt();
		assert(outSlot >= 0);

		PostProcessOutputType outType = (PostProcessOutputType)out_json->GetMember(KPostProcessOutputData::msTypeKey)->GetInt();

		switch (outType)
		{
			case POST_PROCESS_OUTPUT_PASS:
			{
				auto out_pass_id = out_json->GetMember(KPostProcessOutputData::msPassKey)->GetString();
				outPass = m_Mgr->GetPass(out_pass_id);
				assert(outPass);

				m_Output.InitAsPass(outPass, outSlot);

				break;
			}
			case POST_PROCESS_OUTPUT_TEXTURE:
			{
				std::string outTexPath = out_json->GetMember(KPostProcessOutputData::msTextureKey)->GetString();
				KRenderGlobal::TextrueManager.Acquire(outTexPath.c_str(), outTexture, false);
				assert(outTexture);

				m_Output.InitAsTexture(outTexture, outSlot);

				break;
			}
			default:
			{
				assert(false && "impossible to reach");
				break;
			}
		}
	}

	inPass->AddInputConnection(this, inSlot);
	if (outPass)
	{
		outPass->AddOutputConnection(this, outSlot);
	}

	return true;
}