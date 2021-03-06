/**
 * Fair warning: Like ALOT of Viper, SourceMod helpers + SDKTools code is reused! Thanks to the entire SM team!
 */

#include "SDKToolsModule.h"
#include "InterfaceContainer.h"
#include "ClientNotConnectedExceptionType.h"
#include "ClientDataNotAvailableExceptionType.h"
#include "IServerNotFoundExceptionType.h"
#include "ClientNotInGameExceptionType.h"
#include "InvalidEdictExceptionType.h"
#include "InvalidEntityExceptionType.h"
#include "LightStyleOutOfRangeExceptionType.h"
#include "SDKToolsModSupportNotAvailableExceptionType.h"
#include "MapMustBeRunningExceptionType.h"
#include "InvalidStringTableExceptionType.h"
#include "InvalidStringTableStringIndexExceptionType.h"
#include "InvalidTempEntExceptionType.h"
#include "NoTempEntCallInProgressExceptionType.h"
#include "InvalidTempEntPropertyExceptionType.h"
#include "InvalidSendPropertyExceptionType.h"
#include "NullReferenceExceptionType.h"
#include "EntityOffsetOutOfRangeExceptionType.h"
#include "TempEntHookDoesNotExistExceptionType.h"
#include "EntityOutputClassNameHook.h"
#include "EntityDataMapInfoType.h"
#include "EntityOutputClassNameHookDoesNotExistExceptionType.h"
#include "InvalidTeamExceptionType.h"
#include "ColorType.h"
#include "VectorType.h"
#include "EntityPropertyFieldTypes.h"
#include "PointContentsType.h"
#include "TraceResultsType.h"
#include "ViperRecipientFilter.h"
#include "NetStatsType.h"
#include "game/server/iplayerinfo.h"
#include "ViperTraceFilter.h"
#include "SDKToolsClientListener.h"
#include <public\iclient.h>
#include <public\cdll_int.h>
#include <public\worldsize.h>
#include "server_class.h"
#include "dt_common.h"
#include "Util.h"
#include "CDetour\detours.h"

namespace py = boost::python;

SourceMod::ICallWrapper *sdktools__AcceptEntityInputCallWrapper = NULL;
SourceMod::ICallWrapper *sdktools__GiveNamedItemCallWrapper = NULL;
SourceMod::ICallWrapper *sdktools__RemovePlayerItemCallWrapper = NULL;
SourceMod::ICallWrapper *sdktools__IgniteEntityCallWrapper = NULL;
SourceMod::ICallWrapper *sdktools__ExtinguishEntityCallWrapper = NULL;
SourceMod::ICallWrapper *sdktools__ForcePlayerSuicideCallWrapper = NULL;
SourceMod::ICallWrapper *sdktools__TeleportEntityCallWrapper = NULL;
SourceMod::ICallWrapper *sdktools__GetClientWeaponSlotCallWrapper = NULL;
SourceMod::ICallWrapper *sdktools__ActivateEntityCallWrapper = NULL;
SourceMod::ICallWrapper *sdktools__EquipPlayerWeaponCallWrapper = NULL;
SourceMod::ICallWrapper *sdktools__SetEntityModelCallWrapper = NULL;
SourceMod::ICallWrapper *sdktools__GetClientEyeAnglesCallWrapper = NULL;
SourceMod::ICallWrapper *sdktools__SetUserCvarCallWrapper = NULL;
SourceMod::ICallWrapper *sdktools__UpdateUserSettingsCallWrapper = NULL;

#if SOURCE_ENGINE < SE_ORANGEBOX
SourceMod::ICallWrapper *sdktools__CreateEntityByNameCallWrapper = NULL;
SourceMod::ICallWrapper *sdktools__DispatchKeyValueCallWrapper = NULL;
SourceMod::ICallWrapper *sdktools__DispatchKeyValueFloatCallWrapper = NULL;
SourceMod::ICallWrapper *sdktools__DispatchKeyValueVectorCallWrapper = NULL;
SourceMod::ICallWrapper *sdktools__DispatchKeyValueVectorCallWrapper = NULL;
SourceMod::ICallWrapper *sdktools__DispatchSpawnCallWrapper = NULL;
#endif

size_t sdktools__VoiceFlags[65];
size_t sdktools__VoiceHookCount = 0;
ListenOverride sdktools__VoiceMap[65][65];
bool sdktools__ClientMutes[65][65];

void *sdktools__TEListHead = NULL;
int sdktools__TENameOffs = 0;
int sdktools__TENextOffs = 0;
int sdktools__TEGetClassNameOffs = 0;

void *sdktools__TECurrentEffect = NULL;
ServerClass *sdktools__TECurrentServerClass = NULL;

SourceMod::ICallWrapper *sdktools__TEGetServerClassCallWrapper = NULL;
std::list<TempEntHook> sdktools__TEHooks;
std::list<EntityOutputClassNameHook> sdktools__EntityOutputClassNameHooks;
bool sdktools__TELoaded = false;

SH_DECL_HOOK3(IVoiceServer, SetClientListening, SH_NOATTRIB, 0, bool, int, int, bool);
SH_DECL_HOOK5_void(IVEngineServer, PlaybackTempEntity, SH_NOATTRIB, 0, IRecipientFilter &, float, const void *, const SendTable *, int);

unsigned char sdktools__VariantTInstance[SIZEOF_VARIANT_T] = {0};

void sdktools__init_variant_t() {
	unsigned char *vptr = sdktools__VariantTInstance;

	*(int *)vptr = 0;
	vptr += sizeof(int)*3;
	*(unsigned long *)vptr = INVALID_EHANDLE_INDEX;
	vptr += sizeof(unsigned long);
	*(fieldtype_t *)vptr = FIELD_VOID;
}

std::string sdktools__GameRulesProxyClassName;

#ifdef PLATFORM_WINDOWS
DETOUR_DECL_MEMBER8(FireOutput, void, int, what, int, the, int, hell, int, msvc, void *, variant_t, CBaseEntity *, pActivator, CBaseEntity *, pCaller, float, fDelay)
{
	sdktools__OnFireOutput((void *)this, pActivator, pCaller, fDelay);
	DETOUR_MEMBER_CALL(FireOutput)(what, the, hell, msvc, variant_t, pActivator, pCaller, fDelay);
}
#else
DETOUR_DECL_MEMBER4(FireOutput, void, void *, variant_t, CBaseEntity *, pActivator, CBaseEntity *, pCaller, float, fDelay)
{
	sdktools__OnFireOutput((void *)this, pActivator, pCaller, fDelay);
	DETOUR_MEMBER_CALL(FireOutput)(variant_t, pActivator, pCaller, fDelay);
}
#endif

struct TeamInfo
{
	const char *ClassName;
	CBaseEntity *pEnt;
};

std::vector<TeamInfo> sdktools__Teams;

void sdktools__inactive_client(int clientIndex) {
	SourceMod::IGamePlayer *player = playerhelpers->GetGamePlayer(clientIndex);
	
	if(!player->IsConnected()) {
		throw ClientNotConnectedExceptionType(clientIndex);
	}

	IServer *server = g_Interfaces.SDKToolsInstance->GetIServer();

	if (NULL == server) {
		throw IServerNotFoundExceptionType();
	}

	IClient *client = server->GetClient(clientIndex);
	
	if(NULL == client) {
		throw ClientDataNotAvailableExceptionType(clientIndex, "iclient");
	}

	client->Inactivate();
}

void sdktools__reconnect_client(int clientIndex) {
	SourceMod::IGamePlayer *player = playerhelpers->GetGamePlayer(clientIndex);
	
	if(!player->IsConnected()) {
		throw ClientNotConnectedExceptionType(clientIndex);
	}

	IServer *server = g_Interfaces.SDKToolsInstance->GetIServer();

	if (NULL == server) {
		throw IServerNotFoundExceptionType();
	}

	IClient *client = server->GetClient(clientIndex);
	
	if(NULL == client) {
		throw ClientDataNotAvailableExceptionType(clientIndex, "iclient");
	}

	client->Reconnect();
}

void sdktools__set_client_view_entity(int clientIndex, int entityIndex) {
	SourceMod::IGamePlayer *player = playerhelpers->GetGamePlayer(clientIndex);

	if(!player->IsConnected()) {
		throw ClientNotConnectedExceptionType(clientIndex);
	}

	if(!player->IsInGame()) {
		throw ClientNotInGameExceptionType(clientIndex);
	}

	edict_t *edict = PEntityOfEntIndex(entityIndex);
	
	if(!edict || edict->IsFree()) {
		throw InvalidEdictExceptionType(entityIndex);
	}

	engine->SetView(player->GetEdict(), edict);
}

void sdktools__set_light_style(int style, std::string lightStr) {
	if(style < 0 || style >= MAX_LIGHTSTYLES) {
		throw LightStyleOutOfRangeExceptionType(style);
	}

	char *str = UTIL_Strdup(lightStr.c_str());

	engine->LightStyle(style, str);
}

VectorType sdktools__get_client_eye_position(int clientIndex) {
	SourceMod::IGamePlayer *player = playerhelpers->GetGamePlayer(clientIndex);

	if(!player->IsConnected()) {
		throw ClientNotConnectedExceptionType(clientIndex);
	}

	if(!player->IsInGame()) {
		throw ClientNotInGameExceptionType(clientIndex);
	}

	Vector p;
	g_Interfaces.ServerGameClientsInstance->ClientEarPosition(player->GetEdict(), &p);

	return VectorType(p.x, p.y, p.z);
}

bool sdktools__accept_entity_input(int entityIndex, std::string action, int activatorIndex = -1, int callerIndex = -1, int outputID = 0) {
	if (NULL == sdktools__AcceptEntityInputCallWrapper) {
		int offset;

		if (!g_Interfaces.GameConfigInstance->GetOffset("AcceptInput", &offset)) {
			throw SDKToolsModSupportNotAvailableExceptionType("AcceptEntityInput");
		}

		PassInfo pass[6];
		pass[0].type = PassType_Basic;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(const char *);
		pass[1].type = pass[2].type = PassType_Basic;
		pass[1].flags = pass[2].flags = PASSFLAG_BYVAL;
		pass[1].size = pass[2].size = sizeof(CBaseEntity *);
		pass[3].type = PassType_Object;
		pass[3].flags = PASSFLAG_BYVAL|PASSFLAG_OCTOR|PASSFLAG_ODTOR|PASSFLAG_OASSIGNOP;
		pass[3].size = SIZEOF_VARIANT_T;
		pass[4].type = PassType_Basic;
		pass[4].flags = PASSFLAG_BYVAL;
		pass[4].size = sizeof(int);
		pass[5].type = PassType_Basic;
		pass[5].flags = PASSFLAG_BYVAL;
		pass[5].size = sizeof(bool);

		if (!(sdktools__AcceptEntityInputCallWrapper = g_Interfaces.BinToolsInstance->CreateVCall(offset, 0, 0, &pass[5], pass, 5))) {
			throw SDKToolsModSupportNotAvailableExceptionType("AcceptEntityInput");
		}
	}

	CBaseEntity *activatorEntity, *callerEntity, *entity;
	
	unsigned char vstk[sizeof(void *) + sizeof(const char *) + sizeof(CBaseEntity *)*2 + SIZEOF_VARIANT_T + sizeof(int)];
	unsigned char *vptr = vstk;

	edict_t *edict = PEntityOfEntIndex(entityIndex);

	if(edict == NULL || edict->IsFree()) {
		throw InvalidEdictExceptionType(entityIndex);
	}

	entity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(edict);

	if(entity == NULL) {
		throw InvalidEntityExceptionType(entityIndex);
	}

	if(activatorIndex == -1) {
		activatorEntity = NULL;
	} else {
		edict_t *activatorEdict = PEntityOfEntIndex(activatorIndex);

		if(activatorEdict == NULL || activatorEdict->IsFree()) {
			throw InvalidEdictExceptionType(activatorIndex);
		}

		activatorEntity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(activatorEdict);

		if(activatorEntity == NULL) {
			throw InvalidEntityExceptionType(activatorIndex);
		}
	}

	if (callerIndex == -1) {
		callerEntity = NULL;
	} else {
		edict_t *callerEdict = PEntityOfEntIndex(callerIndex);

		if(callerEdict == NULL || callerEdict->IsFree()) {
			throw InvalidEdictExceptionType(callerIndex);
		}

		callerEntity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(callerEdict);

		if(callerEntity == NULL) {
			throw InvalidEntityExceptionType(callerIndex);
		}
	}

	*(void **)vptr = entity;
	vptr += sizeof(void *);
	*(const char **)vptr = action.c_str();
	vptr += sizeof(const char *);
	*(CBaseEntity **)vptr = activatorEntity;
	vptr += sizeof(CBaseEntity *);
	*(CBaseEntity **)vptr = callerEntity;
	vptr += sizeof(CBaseEntity *);
	memcpy(vptr, sdktools__VariantTInstance, SIZEOF_VARIANT_T);
	vptr += SIZEOF_VARIANT_T;
	*(int *)vptr = outputID;

	bool ret;
	sdktools__AcceptEntityInputCallWrapper->Execute(vstk, &ret);

	sdktools__init_variant_t();

	return ret;
}

void sdktools__set_variant_bool(bool value) {
	unsigned char *vptr = sdktools__VariantTInstance;

	*(bool *)vptr = value;
	vptr += sizeof(int)*3 + sizeof(unsigned long);
	*(fieldtype_t *)vptr = FIELD_BOOLEAN;
}

void sdktools__set_variant_string(std::string value) {
	unsigned char *vptr = sdktools__VariantTInstance;

	*(string_t *)vptr = MAKE_STRING(value.c_str());
	vptr += sizeof(int)*3 + sizeof(unsigned long);
	*(fieldtype_t *)vptr = FIELD_STRING;
}

void sdktools__set_variant_int(int value) {
	unsigned char *vptr = sdktools__VariantTInstance;

	*(int *)vptr = value;
	vptr += sizeof(int)*3 + sizeof(unsigned long);
	*(fieldtype_t *)vptr = FIELD_INTEGER;
}

void sdktools__set_variant_float(float value) {
	unsigned char *vptr = sdktools__VariantTInstance;

	*(float *)vptr = value;
	vptr += sizeof(int)*3 + sizeof(unsigned long);
	*(fieldtype_t *)vptr = FIELD_FLOAT;
}

void sdktools__set_variant_vector(VectorType value) {
	unsigned char *vptr = sdktools__VariantTInstance;

	*(float *)vptr = value.X;
	vptr += sizeof(float);
	*(float *)vptr = value.Y;
	vptr += sizeof(float);
	*(float *)vptr = value.Z;
	vptr += sizeof(float) + sizeof(unsigned long);
	*(fieldtype_t *)vptr = FIELD_VECTOR;
}

void sdktools__set_variant_position_vector(VectorType value) {
	unsigned char *vptr = sdktools__VariantTInstance;

	*(float *)vptr = value.X;
	vptr += sizeof(float);
	*(float *)vptr = value.Y;
	vptr += sizeof(float);
	*(float *)vptr = value.Z;
	vptr += sizeof(float) + sizeof(unsigned long);
	*(fieldtype_t *)vptr = FIELD_POSITION_VECTOR;
}

void sdktools__set_variant_color(ColorType value) {
	unsigned char *vptr = sdktools__VariantTInstance;

	*(unsigned char *)vptr = value.Red;
	vptr += sizeof(unsigned char);
	*(unsigned char *)vptr = value.Green;
	vptr += sizeof(unsigned char);
	*(unsigned char *)vptr = value.Blue;
	vptr += sizeof(unsigned char);
	*(unsigned char *)vptr = value.Alpha;
	vptr += sizeof(unsigned char) + sizeof(int)*2 + sizeof(unsigned long);
	*(fieldtype_t *)vptr = FIELD_COLOR32;
}

void sdktools__set_variant_entity(int value) {
	edict_t *edict = PEntityOfEntIndex(value);
	
	if(edict == NULL || edict->IsFree()) {
		throw InvalidEdictExceptionType(value);
	}

	CBaseEntity *entity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(edict);
	
	if(entity == NULL) {
		throw InvalidEntityExceptionType(value);
	}

	unsigned char *vptr = sdktools__VariantTInstance;
	CBaseHandle baseHandle = reinterpret_cast<IHandleEntity *>(entity)->GetRefEHandle();

	vptr += sizeof(int)*3;
	*(unsigned long *)vptr = (unsigned long)(baseHandle.ToInt());
	vptr += sizeof(unsigned long);
	*(fieldtype_t *)vptr = FIELD_EHANDLE;
}

int sdktools__give_client_item(int clientIndex, std::string itemName, int subType = 0) {
	if (NULL == sdktools__GiveNamedItemCallWrapper) {
		int offset;
		void *addr;

		PassInfo pass[3];
		pass[0].type = PassType_Basic;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(const char *);
		pass[1].type = PassType_Basic;
		pass[1].flags = PASSFLAG_BYVAL;
		pass[1].size = sizeof(int);
		pass[2].type = PassType_Basic;
		pass[2].flags = PASSFLAG_BYVAL;
		pass[2].size = sizeof(CBaseEntity *);

		if (g_Interfaces.GameConfigInstance->GetOffset("GiveNamedItem", &offset)) {
			sdktools__GiveNamedItemCallWrapper = g_Interfaces.BinToolsInstance->CreateVCall(offset, 0, 0, &pass[2], pass, 2);
		}
		else if(g_Interfaces.GameConfigInstance->GetMemSig("GiveNamedItem", &addr)) {
			sdktools__GiveNamedItemCallWrapper = g_Interfaces.BinToolsInstance->CreateCall(addr, CallConv_ThisCall, &pass[2], pass, 2);
		}
		else {
			throw SDKToolsModSupportNotAvailableExceptionType("GiveNamedItem");
		}
	}

	SourceMod::IGamePlayer *player = playerhelpers->GetGamePlayer(clientIndex);

	if(!player->IsConnected()) {
		throw ClientNotConnectedExceptionType(clientIndex);
	}

	if(!player->IsInGame()) {
		throw ClientNotInGameExceptionType(clientIndex);
	}
	
	CBaseEntity *entity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(player->GetEdict());

	if(entity == NULL) {
		throw InvalidEntityExceptionType(clientIndex);
	}

	unsigned char vstk[sizeof(void *) + sizeof(const char *) + sizeof(int)];
	unsigned char *vptr = vstk;

	*(void **)vptr = entity;
	vptr += sizeof(void *);
	*(const char **)vptr = itemName.c_str();
	vptr += sizeof(const char *);
	*(int *)vptr = subType;

	CBaseEntity *ret;
	sdktools__GiveNamedItemCallWrapper->Execute(vstk, &ret);

	if(ret == NULL) {
		return -1;
	}

	edict_t *newEdict = g_Interfaces.ServerGameEntsInstance->BaseEntityToEdict(ret);

	if(newEdict == NULL || newEdict->IsFree()) {
		return -1;
	}

	int newEntityIndex = IndexOfEdict(newEdict);

	return newEntityIndex;
}

bool sdktools__remove_client_item(int clientIndex, int itemEntityIndex) {
	if (NULL == sdktools__RemovePlayerItemCallWrapper) {
		int offset;
		void *addr;

		PassInfo pass[2];
		pass[0].type = PassType_Basic;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(CBaseEntity *);
		pass[1].type = PassType_Basic;
		pass[1].flags = PASSFLAG_BYVAL;
		pass[1].size = sizeof(bool);

		if (g_Interfaces.GameConfigInstance->GetOffset("RemovePlayerItem", &offset)) {
			sdktools__RemovePlayerItemCallWrapper = g_Interfaces.BinToolsInstance->CreateVCall(offset, 0, 0, &pass[1], pass, 1);
		}
		else if(g_Interfaces.GameConfigInstance->GetMemSig("RemovePlayerItem", &addr)) {
			sdktools__RemovePlayerItemCallWrapper = g_Interfaces.BinToolsInstance->CreateCall(addr, CallConv_ThisCall, &pass[1], pass, 1);
		}
		else {
			throw SDKToolsModSupportNotAvailableExceptionType("RemovePlayerItem");
		}
	}

	SourceMod::IGamePlayer *player = playerhelpers->GetGamePlayer(clientIndex);

	if(!player->IsConnected()) {
		throw ClientNotConnectedExceptionType(clientIndex);
	}

	if(!player->IsInGame()) {
		throw ClientNotInGameExceptionType(clientIndex);
	}
	
	CBaseEntity *entity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(player->GetEdict());

	if(entity == NULL) {
		throw InvalidEntityExceptionType(clientIndex);
	}

	edict_t *itemEdict = PEntityOfEntIndex(itemEntityIndex);

	if(NULL == itemEdict || itemEdict->IsFree()) {
		throw InvalidEdictExceptionType(itemEntityIndex);
	}

	CBaseEntity *itemEntity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(itemEdict);

	if(NULL == itemEntity) {
		throw InvalidEntityExceptionType(itemEntityIndex);
	}

	unsigned char vstk[sizeof(CBaseEntity *) + sizeof(CBaseEntity *)];
	unsigned char *vptr = vstk;

	*(CBaseEntity **)vptr = entity;
	vptr += sizeof(CBaseEntity *);
	*(CBaseEntity **)vptr = itemEntity;
	
	bool ret;

	sdktools__RemovePlayerItemCallWrapper->Execute(vstk, &ret);
	
	return ret;
}

#if SOURCE_ENGINE != SE_DARKMESSIAH
void sdktools__ignite_entity(int entityIndex, float time = 0.0f, bool npc = false, float size = 0.0f, bool level = false) {
	if (NULL == sdktools__IgniteEntityCallWrapper) {
		int offset;
		void *addr;

		PassInfo pass[4];
		pass[0].type = PassType_Float;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(float);
		pass[1].type = PassType_Basic;
		pass[1].flags = PASSFLAG_BYVAL;
		pass[1].size = sizeof(bool);
		pass[2].type = PassType_Float;
		pass[2].flags = PASSFLAG_BYVAL;
		pass[2].size = sizeof(float);
		pass[3].type = PassType_Basic;
		pass[3].flags = PASSFLAG_BYVAL;
		pass[3].size = sizeof(bool);

		if (g_Interfaces.GameConfigInstance->GetOffset("Ignite", &offset)) {
			sdktools__IgniteEntityCallWrapper = g_Interfaces.BinToolsInstance->CreateVCall(offset, 0, 0, NULL, pass, 4);
		}
		else if(g_Interfaces.GameConfigInstance->GetMemSig("Ignite", &addr)) {
			sdktools__IgniteEntityCallWrapper = g_Interfaces.BinToolsInstance->CreateCall(addr, CallConv_ThisCall, NULL, pass, 4);
		}
		else {
			throw SDKToolsModSupportNotAvailableExceptionType("Ignite");
		}
	}

	edict_t *edict = PEntityOfEntIndex(entityIndex);

	if(edict == NULL || edict->IsFree()) {
		throw InvalidEdictExceptionType(entityIndex);
	}

	CBaseEntity *entity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(edict);

	if(entity == NULL) {
		throw InvalidEntityExceptionType(entityIndex);
	}

	unsigned char vstk[sizeof(CBaseEntity *) + sizeof(float) + sizeof(bool) + sizeof(float) + sizeof(bool)];
	unsigned char *vptr = vstk;

	*(CBaseEntity **)vptr = entity;
	vptr += sizeof(CBaseEntity *);
	*(float *)vptr = time;
	vptr += sizeof(float);
	*(bool *)vptr = npc;
	vptr += sizeof(bool);
	*(float *)vptr = size;
	vptr += sizeof(float);
	*(bool *)vptr = level;
	
	sdktools__IgniteEntityCallWrapper->Execute(vstk, NULL);
}
#else
#endif

void sdktools__extinguish_entity(int entityIndex) {
	if (NULL == sdktools__ExtinguishEntityCallWrapper) {
		int offset;
		void *addr;

		if (g_Interfaces.GameConfigInstance->GetOffset("Extinguish", &offset)) {
			sdktools__ExtinguishEntityCallWrapper = g_Interfaces.BinToolsInstance->CreateVCall(offset, 0, 0, NULL, NULL, 0);
		}
		else if(g_Interfaces.GameConfigInstance->GetMemSig("Extinguish", &addr)) {
			sdktools__ExtinguishEntityCallWrapper = g_Interfaces.BinToolsInstance->CreateCall(addr, CallConv_ThisCall, NULL, NULL, 0);
		}
		else {
			throw SDKToolsModSupportNotAvailableExceptionType("Extinguish");
		}
	}

	edict_t *edict = PEntityOfEntIndex(entityIndex);

	if(edict == NULL || edict->IsFree()) {
		throw InvalidEdictExceptionType(entityIndex);
	}

	CBaseEntity *entity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(edict);

	if(entity == NULL) {
		throw InvalidEntityExceptionType(entityIndex);
	}

	unsigned char vstk[sizeof(CBaseEntity *)];
	unsigned char *vptr = vstk;

	*(CBaseEntity **)vptr = entity;
	
	sdktools__ExtinguishEntityCallWrapper->Execute(vstk, NULL);
}

class CBasePlayer;

void sdktools__force_client_suicide(int clientIndex, bool explode = false, bool force = false) {
	if (NULL == sdktools__ForcePlayerSuicideCallWrapper) {
		int offset;
		void *addr;

		PassInfo pass[2];
		pass[0].type = PassType_Basic;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(bool);
		pass[1].type = PassType_Basic;
		pass[1].flags = PASSFLAG_BYVAL;
		pass[1].size = sizeof(bool);

		if (g_Interfaces.GameConfigInstance->GetOffset("CommitSuicide", &offset)) {
			sdktools__ForcePlayerSuicideCallWrapper = g_Interfaces.BinToolsInstance->CreateVCall(offset, 0, 0, NULL, pass, 2);
		}
		else if(g_Interfaces.GameConfigInstance->GetMemSig("CommitSuicide", &addr)) {
			sdktools__ForcePlayerSuicideCallWrapper = g_Interfaces.BinToolsInstance->CreateCall(addr, CallConv_ThisCall, NULL, pass, 2);
		}
		else {
			throw SDKToolsModSupportNotAvailableExceptionType("CommitSuicide");
		}
	}

	SourceMod::IGamePlayer *player = playerhelpers->GetGamePlayer(clientIndex);

	if(!player->IsConnected()) {
		throw ClientNotConnectedExceptionType(clientIndex);
	}

	if(!player->IsInGame()) {
		throw ClientNotInGameExceptionType(clientIndex);
	}
	
	CBasePlayer *basePlayer = (CBasePlayer*)g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(player->GetEdict());

	if(basePlayer == NULL) {
		throw InvalidEntityExceptionType(clientIndex);
	}

	unsigned char vstk[sizeof(CBasePlayer *) + sizeof(bool) + sizeof(bool)];
	unsigned char *vptr = vstk;

	*(CBasePlayer **)vptr = basePlayer;
	vptr += sizeof(CBasePlayer *);
	*(bool *)vptr = explode;
	vptr += sizeof(bool);
	*(bool *)vptr = force;
	
	sdktools__ForcePlayerSuicideCallWrapper->Execute(vstk, NULL);
}

void sdktools__teleport_entity(int entityIndex, py::object origin = BOOST_PY_NONE, py::object angles = BOOST_PY_NONE, py::object velocity = BOOST_PY_NONE) {
	if (NULL == sdktools__TeleportEntityCallWrapper) {
		int offset;
		void *addr;

		PassInfo pass[3];
		pass[0].type = PassType_Basic;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(Vector *);
		pass[1].type = PassType_Basic;
		pass[1].flags = PASSFLAG_BYVAL;
		pass[1].size = sizeof(QAngle *);
		pass[2].type = PassType_Basic;
		pass[2].flags = PASSFLAG_BYVAL;
		pass[2].size = sizeof(Vector *);
		
		if (g_Interfaces.GameConfigInstance->GetOffset("Teleport", &offset)) {
			sdktools__TeleportEntityCallWrapper = g_Interfaces.BinToolsInstance->CreateVCall(offset, 0, 0, NULL, pass, 3);
		}
		else if(g_Interfaces.GameConfigInstance->GetMemSig("Teleport", &addr)) {
			sdktools__TeleportEntityCallWrapper = g_Interfaces.BinToolsInstance->CreateCall(addr, CallConv_ThisCall, NULL, pass, 3);
		}
		else {
			throw SDKToolsModSupportNotAvailableExceptionType("Teleport");
		}
	}

	edict_t *edict = PEntityOfEntIndex(entityIndex);

	if(edict == NULL || edict->IsFree()) {
		throw InvalidEdictExceptionType(entityIndex);
	}

	CBaseEntity *entity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(edict);

	if(entity == NULL) {
		throw InvalidEntityExceptionType(entityIndex);
	}

	// Parse out the possibily NULL arguments
	Vector *originVec = NULL;

	if(origin != BOOST_PY_NONE) {
		VectorType o = py::extract<VectorType>(origin);
		originVec = new Vector(o.X, o.Y, o.Z);
	}

	QAngle *anglesQAng = NULL;

	if(angles != BOOST_PY_NONE) {
		VectorType a = py::extract<VectorType>(angles);
		anglesQAng = new QAngle(a.X, a.Y, a.Z);
	}

	Vector *velocityVec = NULL;

	if(velocity != BOOST_PY_NONE) {
		VectorType v = py::extract<VectorType>(velocity);
		velocityVec = new Vector(v.X, v.Y, v.Z);
	}

	unsigned char vstk[sizeof(CBaseEntity *) + sizeof(Vector *) + sizeof(QAngle *) + sizeof(Vector *)];
	unsigned char *vptr = vstk;

	*(CBaseEntity **)vptr = entity;
	vptr += sizeof(CBaseEntity *);
	*(Vector **)vptr = originVec;
	vptr += sizeof(Vector *);
	*(QAngle **)vptr = anglesQAng;
	vptr += sizeof(QAngle *);
	*(Vector **)vptr = velocityVec;
	
	sdktools__TeleportEntityCallWrapper->Execute(vstk, NULL);

	if(originVec) {
		delete originVec;
	}

	if(anglesQAng) {
		delete anglesQAng;
	}

	if(velocityVec) {
		delete velocityVec;
	}
}

int sdktools__get_client_weapon_slot(int clientIndex, int weaponSlot) {
	if (NULL == sdktools__GetClientWeaponSlotCallWrapper) {
		int offset;
		void *addr;

		PassInfo pass[2];
		pass[0].type = PassType_Basic;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(int);
		pass[1].type = PassType_Basic;
		pass[1].flags = PASSFLAG_BYVAL;
		pass[1].size = sizeof(CBaseEntity *);
		
		if (g_Interfaces.GameConfigInstance->GetOffset("Weapon_GetSlot", &offset)) {
			sdktools__GetClientWeaponSlotCallWrapper = g_Interfaces.BinToolsInstance->CreateVCall(offset, 0, 0, &pass[1], pass, 1);
		}
		else if(g_Interfaces.GameConfigInstance->GetMemSig("Weapon_GetSlot", &addr)) {
			sdktools__GetClientWeaponSlotCallWrapper = g_Interfaces.BinToolsInstance->CreateCall(addr, CallConv_ThisCall, &pass[1], pass, 1);
		}
		else {
			throw SDKToolsModSupportNotAvailableExceptionType("Weapon_GetSlot");
		}
	}

	SourceMod::IGamePlayer *player = playerhelpers->GetGamePlayer(clientIndex);

	if(!player->IsConnected()) {
		throw ClientNotConnectedExceptionType(clientIndex);
	}

	if(!player->IsInGame()) {
		throw ClientNotInGameExceptionType(clientIndex);
	}
	
	CBasePlayer *basePlayer = (CBasePlayer*)g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(player->GetEdict());

	if(basePlayer == NULL) {
		throw InvalidEntityExceptionType(clientIndex);
	}
	
	unsigned char vstk[sizeof(CBasePlayer *) + sizeof(int)];
	unsigned char *vptr = vstk;

	*(CBasePlayer **)vptr = basePlayer;
	vptr += sizeof(CBasePlayer *);
	*(int *)vptr = weaponSlot;
	
	CBaseEntity *ret;

	sdktools__GetClientWeaponSlotCallWrapper->Execute(vstk, &ret);

	if(ret == NULL) {
		return -1;
	}

	edict_t *retEdict = g_Interfaces.ServerGameEntsInstance->BaseEntityToEdict(ret);

	if(retEdict == NULL || retEdict->IsFree()) {
		return -1;
	}

	return IndexOfEdict(retEdict);
}

void sdktools__activate_entity(int entityIndex) {
	if (NULL == sdktools__ActivateEntityCallWrapper) {
		int offset;
		void *addr;

		
		if (g_Interfaces.GameConfigInstance->GetOffset("Activate", &offset)) {
			sdktools__GetClientWeaponSlotCallWrapper = g_Interfaces.BinToolsInstance->CreateVCall(offset, 0, 0, NULL, NULL, 0);
		}
		else if(g_Interfaces.GameConfigInstance->GetMemSig("Activate", &addr)) {
			sdktools__GetClientWeaponSlotCallWrapper = g_Interfaces.BinToolsInstance->CreateCall(addr, CallConv_ThisCall, NULL, NULL, 0);
		}
		else {
			throw SDKToolsModSupportNotAvailableExceptionType("Activate");
		}
	}

	edict_t *edict = PEntityOfEntIndex(entityIndex);

	if(edict == NULL || edict->IsFree()) {
		throw InvalidEdictExceptionType(entityIndex);
	}

	CBaseEntity *entity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(edict);

	if(entity == NULL) {
		throw InvalidEntityExceptionType(entityIndex);
	}
	
	unsigned char vstk[sizeof(CBaseEntity *)];
	unsigned char *vptr = vstk;

	*(CBaseEntity **)vptr = entity;
	
	sdktools__ActivateEntityCallWrapper->Execute(vstk, NULL);
}

void sdktools__equip_client_weapon(int clientIndex, int weaponEntityIndex) {
	if (NULL == sdktools__EquipPlayerWeaponCallWrapper) {
		int offset;
		void *addr;

		PassInfo pass[1];
		pass[0].type = PassType_Basic;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(CBaseEntity *);

		if (g_Interfaces.GameConfigInstance->GetOffset("WeaponEquip", &offset)) {
			sdktools__EquipPlayerWeaponCallWrapper = g_Interfaces.BinToolsInstance->CreateVCall(offset, 0, 0, NULL, pass, 1);
		}
		else if(g_Interfaces.GameConfigInstance->GetMemSig("WeaponEquip", &addr)) {
			sdktools__EquipPlayerWeaponCallWrapper = g_Interfaces.BinToolsInstance->CreateCall(addr, CallConv_ThisCall, NULL, pass, 1);
		}
		else {
			throw SDKToolsModSupportNotAvailableExceptionType("WeaponEquip");
		}
	}

	SourceMod::IGamePlayer *player = playerhelpers->GetGamePlayer(clientIndex);

	if(!player->IsConnected()) {
		throw ClientNotConnectedExceptionType(clientIndex);
	}

	if(!player->IsInGame()) {
		throw ClientNotInGameExceptionType(clientIndex);
	}
	
	CBasePlayer *basePlayer = (CBasePlayer*)g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(player->GetEdict());

	if(basePlayer == NULL) {
		throw InvalidEntityExceptionType(clientIndex);
	}

	edict_t *weaponEdict = PEntityOfEntIndex(weaponEntityIndex);

	if(weaponEdict == NULL || weaponEdict->IsFree()) {
		throw InvalidEdictExceptionType(weaponEntityIndex);
	}

	CBaseEntity *weaponEntity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(weaponEdict);

	if(weaponEntity == NULL) {
		throw InvalidEntityExceptionType(weaponEntityIndex);
	}
	
	unsigned char vstk[sizeof(CBasePlayer*) + sizeof(CBaseEntity*)];
	unsigned char *vptr = vstk;

	*(CBasePlayer **)vptr = basePlayer;
	vptr += sizeof(CBasePlayer *);
	*(CBaseEntity **)vptr = weaponEntity;
	
	sdktools__EquipPlayerWeaponCallWrapper->Execute(vstk, NULL);
}

#if SOURCE_ENGINE >= SE_ORANGEBOX
int sdktools__create_entity_by_name(std::string name) {
	if(!g_pSM->IsMapRunning()) {
		throw MapMustBeRunningExceptionType();
	}

	CBaseEntity *entity = (CBaseEntity*)g_Interfaces.ServerToolsInstance->CreateEntityByName(name.c_str());

	if(entity == NULL) {
		return -1;
	}

	edict_t *edict = g_Interfaces.ServerGameEntsInstance->BaseEntityToEdict(entity);

	if(edict == NULL || edict->IsFree()) {
		return -1;
	}

	return IndexOfEdict(edict);
}
#else
int sdktools__create_entity_by_name(std::string name) {
	if(!g_pSM->IsMapRunning()) {
		throw MapMustBeRunningExceptionType();
	}

	if (NULL == sdktools__CreateEntityByNameCallWrapper) {
		void *addr;

		PassInfo pass[3];
		pass[0].type = PassType_Basic;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(const char *);
		pass[1].type = PassType_Basic;
		pass[1].flags = PASSFLAG_BYVAL;
		pass[1].size = sizeof(int);
		pass[2].type = PassType_Basic;
		pass[2].flags = PASSFLAG_BYVAL;
		pass[2].size = sizeof(CBaseEntity *);

		if(g_Interfaces.GameConfigInstance->GetMemSig("CreateEntityByName", &addr)) {
			sdktools__CreateEntityByNameCallWrapper = g_Interfaces.BinToolsInstance->CreateCall(addr, CallConv_Cdecl, &pass[2], pass, 2);
		}
		else {
			throw SDKToolsModSupportNotAvailableExceptionType("CreateEntityByName");
		}
	}

	unsigned char vstk[sizeof(const char *) + sizeof(int)];
	unsigned char *vptr = vstk;

	*(const char **)vptr = name.c_str();
	vptr += sizeof(const char *);
	*(int *)vptr = -1;
	
	CBaseEntity *ret;

	sdktools__CreateEntityByNameCallWrapper->Execute(vstk, &ret);

	if(ret == NULL) {
		return -1;
	}

	edict_t *edict = g_Interfaces.ServerGameEntsInstance->BaseEntityToEdict(ret);

	if(edict == NULL || edict->IsFree()) {
		return -1;
	}

	return IndexOfEdict(edict);
}
#endif

#if SOURCE_ENGINE >= SE_ORANGEBOX
bool sdktools__dispatch_key_value(int entityIndex, std::string key, std::string value) {
	edict_t *edict = PEntityOfEntIndex(entityIndex);

	if(edict == NULL || edict->IsFree()) {
		throw InvalidEdictExceptionType(entityIndex);
	}

	CBaseEntity *entity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(edict);

	if(NULL == entity) {
		throw InvalidEntityExceptionType(entityIndex);
	}

	return g_Interfaces.ServerToolsInstance->SetKeyValue(entity, key.c_str(), value.c_str());
}
#else
bool sdktools__dispatch_key_value(int entityIndex, std::string key, std::string value) {
	if (NULL == sdktools__DispatchKeyValueCallWrapper) {
		int offset;
		void *addr;

		PassInfo pass[3];
		pass[0].type = PassType_Basic;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(const char *);
		pass[1].type = PassType_Basic;
		pass[1].flags = PASSFLAG_BYVAL;
		pass[1].size = sizeof(const char *);
		pass[2].type = PassType_Basic;
		pass[2].flags = PASSFLAG_BYVAL;
		pass[2].size = sizeof(bool);
		
		if (g_Interfaces.GameConfigInstance->GetOffset("DispatchKeyValue", &offset)) {
			sdktools__DispatchKeyValueCallWrapper = g_Interfaces.BinToolsInstance->CreateVCall(offset, 0, 0, &pass[2], pass, 2);
		}
		else if(g_Interfaces.GameConfigInstance->GetMemSig("DispatchKeyValue", &addr)) {
			sdktools__DispatchKeyValueCallWrapper = g_Interfaces.BinToolsInstance->CreateCall(addr, CallConv_ThisCall, &pass[2], pass, 2);
		}
		else {
			throw SDKToolsModSupportNotAvailableExceptionType("DispatchKeyValue");
		}
	}

	edict_t *edict = PEntityOfEntIndex(entityIndex);

	if(edict == NULL || edict->IsFree()) {
		throw InvalidEdictExceptionType(entityIndex);
	}

	CBaseEntity *entity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(edict);

	if(entity == NULL) {
		throw InvalidEntityExceptionType(entityIndex);
	}

	unsigned char vstk[sizeof(CBaseEntity *) + sizeof(const char *) + sizeof(const char *)];
	unsigned char *vptr = vstk;

	*(CBaseEntity **)vptr = entity;
	vptr += sizeof(CBaseEntity *);
	*(const char **)vptr = key.c_str();
	vptr += sizeof(const char *);
	*(const char **)vptr = value.c_str();
	
	bool ret;

	sdktools__DispatchKeyValueCallWrapper->Execute(vstk, &ret);

	return ret;
}
#endif

#if SOURCE_ENGINE >= SE_ORANGEBOX
bool sdktools__dispatch_key_value_float(int entityIndex, std::string key, float value) {
	edict_t *edict = PEntityOfEntIndex(entityIndex);

	if(edict == NULL || edict->IsFree()) {
		throw InvalidEdictExceptionType(entityIndex);
	}

	CBaseEntity *entity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(edict);

	if(NULL == entity) {
		throw InvalidEntityExceptionType(entityIndex);
	}

	return g_Interfaces.ServerToolsInstance->SetKeyValue(entity, key.c_str(), value);
}
#else
bool sdktools__dispatch_key_value_float(int entityIndex, std::string key, float value) {
	if (NULL == sdktools__DispatchKeyValueFloatCallWrapper) {
		int offset;
		void *addr;

		PassInfo pass[3];
		pass[0].type = PassType_Basic;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(const char *);
		pass[1].type = PassType_Float;
		pass[1].flags = PASSFLAG_BYVAL;
		pass[1].size = sizeof(float);
		pass[2].type = PassType_Basic;
		pass[2].flags = PASSFLAG_BYVAL;
		pass[2].size = sizeof(bool);
		
		if (g_Interfaces.GameConfigInstance->GetOffset("DispatchKeyValueFloat", &offset)) {
			sdktools__DispatchKeyValueFloatCallWrapper = g_Interfaces.BinToolsInstance->CreateVCall(offset, 0, 0, &pass[2], pass, 2);
		}
		else if(g_Interfaces.GameConfigInstance->GetMemSig("DispatchKeyValueFloat", &addr)) {
			sdktools__DispatchKeyValueFloatCallWrapper = g_Interfaces.BinToolsInstance->CreateCall(addr, CallConv_ThisCall, &pass[2], pass, 2);
		}
		else {
			throw SDKToolsModSupportNotAvailableExceptionType("DispatchKeyValueFloat");
		}
	}

	edict_t *edict = PEntityOfEntIndex(entityIndex);

	if(edict == NULL || edict->IsFree()) {
		throw InvalidEdictExceptionType(entityIndex);
	}

	CBaseEntity *entity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(edict);

	if(entity == NULL) {
		throw InvalidEntityExceptionType(entityIndex);
	}

	unsigned char vstk[sizeof(CBaseEntity *) + sizeof(const char *) + sizeof(const char *)];
	unsigned char *vptr = vstk;

	*(CBaseEntity **)vptr = entity;
	vptr += sizeof(CBaseEntity *);
	*(const char **)vptr = key.c_str();
	vptr += sizeof(const char *);
	*(float *)vptr = value;
	
	bool ret;

	sdktools__DispatchKeyValueFloatCallWrapper->Execute(vstk, &ret);

	return ret;
}
#endif

#if SOURCE_ENGINE >= SE_ORANGEBOX
bool sdktools__dispatch_key_value_vector(int entityIndex, std::string key, VectorType value) {
	edict_t *edict = PEntityOfEntIndex(entityIndex);

	if(edict == NULL || edict->IsFree()) {
		throw InvalidEdictExceptionType(entityIndex);
	}

	CBaseEntity *entity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(edict);

	if(NULL == entity) {
		throw InvalidEntityExceptionType(entityIndex);
	}

	Vector v(value.X, value.Y, value.Z);

	return g_Interfaces.ServerToolsInstance->SetKeyValue(entity, key.c_str(), v);
}
#else
bool sdktools__dispatch_key_value_vector(int entityIndex, std::string key, VectorType value) {
	if (NULL == sdktools__DispatchKeyValueVectorCallWrapper) {
		int offset;
		void *addr;

		PassInfo pass[3];
		pass[0].type = PassType_Basic;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(const char *);
		pass[1].type = PassType_Object;
		pass[1].flags = PASSFLAG_BYVAL|PASSFLAG_OCTOR|PASSFLAG_OASSIGNOP;
		pass[1].size = sizeof(Vector *);
		pass[2].type = PassType_Basic;
		pass[2].flags = PASSFLAG_BYVAL;
		pass[2].size = sizeof(bool);
		
		if (g_Interfaces.GameConfigInstance->GetOffset("DispatchKeyValueVector", &offset)) {
			sdktools__DispatchKeyValueVectorCallWrapper = g_Interfaces.BinToolsInstance->CreateVCall(offset, 0, 0, &pass[2], pass, 2);
		}
		else if(g_Interfaces.GameConfigInstance->GetMemSig("DispatchKeyValueVector", &addr)) {
			sdktools__DispatchKeyValueVectorCallWrapper = g_Interfaces.BinToolsInstance->CreateCall(addr, CallConv_ThisCall, &pass[2], pass, 2);
		}
		else {
			throw SDKToolsModSupportNotAvailableExceptionType("DispatchKeyValueVector");
		}
	}

	edict_t *edict = PEntityOfEntIndex(entityIndex);

	if(edict == NULL || edict->IsFree()) {
		throw InvalidEdictExceptionType(entityIndex);
	}

	CBaseEntity *entity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(edict);

	if(entity == NULL) {
		throw InvalidEntityExceptionType(entityIndex);
	}

	unsigned char vstk[sizeof(CBaseEntity *) + sizeof(const char *) + sizeof(Vector *)];
	unsigned char *vptr = vstk;

	Vector v(value.X, value.Y, value.Z);

	*(CBaseEntity **)vptr = entity;
	vptr += sizeof(CBaseEntity *);
	*(const char **)vptr = key.c_str();
	vptr += sizeof(const char *);
	*(Vector **)vptr = &v;
	
	bool ret;

	sdktools__DispatchKeyValueVectorCallWrapper->Execute(vstk, &ret);

	return ret;
}
#endif

#if SOURCE_ENGINE >= SE_ORANGEBOX
bool sdktools__dispatch_spawn(int entityIndex) {
	edict_t *edict = PEntityOfEntIndex(entityIndex);

	if(edict == NULL || edict->IsFree()) {
		throw InvalidEdictExceptionType(entityIndex);
	}

	CBaseEntity *entity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(edict);

	if(NULL == entity) {
		throw InvalidEntityExceptionType(entityIndex);
	}

	g_Interfaces.ServerToolsInstance->DispatchSpawn(entity);

	return true;
}
#else
bool sdktools__dispatch_spawn(int entityIndex) {
	if (NULL == sdktools__DispatchSpawnCallWrapper) {
		void *addr;

		PassInfo pass[2];
		pass[0].type = PassType_Basic;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(CBaseEntity *);
		pass[1].type = PassType_Basic;
		pass[1].flags = PASSFLAG_BYVAL;
		pass[1].size = sizeof(int);
		
		if(g_Interfaces.GameConfigInstance->GetMemSig("DispatchSpawn", &addr)) {
			sdktools__DispatchSpawnCallWrapper = g_Interfaces.BinToolsInstance->CreateCall(addr, CallConv_Cdecl, &pass[1], pass, 1);
		}
		else {
			throw SDKToolsModSupportNotAvailableExceptionType("DispatchSpawn");
		}
	}

	edict_t *edict = PEntityOfEntIndex(entityIndex);

	if(edict == NULL || edict->IsFree()) {
		throw InvalidEdictExceptionType(entityIndex);
	}

	CBaseEntity *entity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(edict);

	if(entity == NULL) {
		throw InvalidEntityExceptionType(entityIndex);
	}

	unsigned char vstk[sizeof(CBaseEntity *)];
	unsigned char *vptr = vstk;

	*(CBaseEntity **)vptr = entity;
	
	int ret;

	sdktools__DispatchSpawnCallWrapper->Execute(vstk, &ret);

	return ret != -1;
}
#endif

void sdktools__set_entity_model(int entityIndex, std::string model) {
	if (NULL == sdktools__SetEntityModelCallWrapper) {
		int offset;
		void *addr;

		PassInfo pass[1];
		pass[0].type = PassType_Basic;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(const char *);
		
		if (g_Interfaces.GameConfigInstance->GetOffset("SetEntityModel", &offset)) {
			sdktools__SetEntityModelCallWrapper = g_Interfaces.BinToolsInstance->CreateVCall(offset, 0, 0, NULL, pass, 1);
		}
		else if(g_Interfaces.GameConfigInstance->GetMemSig("SetEntityModel", &addr)) {
			sdktools__SetEntityModelCallWrapper = g_Interfaces.BinToolsInstance->CreateCall(addr, CallConv_ThisCall, NULL, pass, 1);
		}
		else {
			throw SDKToolsModSupportNotAvailableExceptionType("SetEntityModel");
		}
	}

	edict_t *edict = PEntityOfEntIndex(entityIndex);

	if(edict == NULL || edict->IsFree()) {
		throw InvalidEdictExceptionType(entityIndex);
	}

	CBaseEntity *entity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(edict);

	if(entity == NULL) {
		throw InvalidEntityExceptionType(entityIndex);
	}

	unsigned char vstk[sizeof(CBaseEntity *) + sizeof(const char*)];
	unsigned char *vptr = vstk;

	*(CBaseEntity **)vptr = entity;
	vptr += sizeof(CBaseEntity *);
	*(const char **)vptr = model.c_str();

	sdktools__SetEntityModelCallWrapper->Execute(vstk, NULL);
}

std::string sdktools__get_client_decal_file(int clientIndex) {
	SourceMod::IGamePlayer *player = playerhelpers->GetGamePlayer(clientIndex);

	if(!player->IsConnected()) {
		throw ClientNotConnectedExceptionType(clientIndex);
	}

	if(!player->IsInGame()) {
		throw ClientNotInGameExceptionType(clientIndex);
	}

	player_info_t info;

	#if SOURCE_ENGINE >= SE_ORANGEBOX
	engine->GetPlayerInfo(clientIndex, &info);
	#else
	IServer *server = g_Interfaces.SDKToolsInstance->GetIServer();

	if(NULL == server) {
		throw IServerNotFoundExceptionType();
	}
	
	server->GetPlayerInfo(clientIndex - 1, &info);
	#endif

	if(!info.customFiles[0]) {
		throw ClientDataNotAvailableExceptionType(clientIndex, "decal");
	}

	char buf[1024];

	Q_binarytohex((byte *)&info.customFiles[0], sizeof(info.customFiles[0]), buf, sizeof(buf));

	return std::string(buf);
}


bool sdktools__lock_string_tables(bool lock) {
	return engine->LockNetworkStringTables(lock);
}

int sdktools__find_string_table(std::string name) {
	INetworkStringTable *stringTable = g_Interfaces.NetworkStringTableContainerInstance->FindTable(name.c_str());

	if (NULL == stringTable) {
		return INVALID_STRING_TABLE;
	}

	return stringTable->GetTableId();
}

int sdktools__get_num_string_tables() {
	return g_Interfaces.NetworkStringTableContainerInstance->GetNumTables();
}

int sdktools__get_string_table_num_strings(int tableIndex) {
	INetworkStringTable *stringTable = g_Interfaces.NetworkStringTableContainerInstance->GetTable(tableIndex);

	if(NULL == stringTable) {
		throw InvalidStringTableExceptionType(tableIndex);
	}

	return stringTable->GetNumStrings();
}

int sdktools__get_string_table_max_strings(int tableIndex) {
	INetworkStringTable *stringTable = g_Interfaces.NetworkStringTableContainerInstance->GetTable(tableIndex);

	if(NULL == stringTable) {
		throw InvalidStringTableExceptionType(tableIndex);
	}

	return stringTable->GetMaxStrings();
}

std::string sdktools__get_string_table_name(int tableIndex) {
	INetworkStringTable *stringTable = g_Interfaces.NetworkStringTableContainerInstance->GetTable(tableIndex);

	if(NULL == stringTable) {
		throw InvalidStringTableExceptionType(tableIndex);
	}

	return std::string(stringTable->GetTableName());
}

int sdktools__find_string_index(int tableIndex, std::string needle) {
	INetworkStringTable *stringTable = g_Interfaces.NetworkStringTableContainerInstance->GetTable(tableIndex);

	if(NULL == stringTable) {
		throw InvalidStringTableExceptionType(tableIndex);
	}

	return stringTable->FindStringIndex(needle.c_str());
}

std::string sdktools__read_string_table(int tableIndex, int stringIndex) {
	INetworkStringTable *stringTable = g_Interfaces.NetworkStringTableContainerInstance->GetTable(tableIndex);

	if(NULL == stringTable) {
		throw InvalidStringTableExceptionType(tableIndex);
	}
	
	const char *str = stringTable->GetString(stringIndex);

	if(NULL == str) {
		throw InvalidStringTableStringIndexExceptionType(tableIndex, stringIndex);
	}

	return std::string(str);
}

int sdktools__get_string_table_data_length(int tableIndex, int stringIndex) {
	INetworkStringTable *stringTable = g_Interfaces.NetworkStringTableContainerInstance->GetTable(tableIndex);

	if(NULL == stringTable) {
		throw InvalidStringTableExceptionType(tableIndex);
	}
	
	if(stringIndex < 0 || stringIndex >= stringTable->GetNumStrings()) {
		throw InvalidStringTableStringIndexExceptionType(tableIndex, stringIndex);
	}

	const void *userdata;
	int datalen;

	userdata = stringTable->GetStringUserData(stringIndex, &datalen);

	if(NULL == userdata) {
		return 0;
	}

	return datalen;
}

std::string sdktools__get_string_table_data(int tableIndex, int stringIndex) {
	INetworkStringTable *stringTable = g_Interfaces.NetworkStringTableContainerInstance->GetTable(tableIndex);

	if(NULL == stringTable) {
		throw InvalidStringTableExceptionType(tableIndex);
	}
	
	if(stringIndex < 0 || stringIndex >= stringTable->GetNumStrings()) {
		throw InvalidStringTableStringIndexExceptionType(tableIndex, stringIndex);
	}

	const char *userdata;
	int datalen;

	userdata = (const char*)stringTable->GetStringUserData(stringIndex, &datalen);

	if(NULL == userdata) {
		return std::string();
	}

	return std::string(userdata);
}

void sdktools__set_string_table_data(int tableIndex, int stringIndex, std::string value, int length = -1) {
	INetworkStringTable *stringTable = g_Interfaces.NetworkStringTableContainerInstance->GetTable(tableIndex);

	if(NULL == stringTable) {
		throw InvalidStringTableExceptionType(tableIndex);
	}
	
	if(stringIndex < 0 || stringIndex >= stringTable->GetNumStrings()) {
		throw InvalidStringTableStringIndexExceptionType(tableIndex, stringIndex);
	}

	if(length == -1) {
		length = value.length();
	}

	stringTable->SetStringUserData(stringIndex, length, value.c_str());
}

void sdktools__add_to_string_table(int tableIndex, std::string str, std::string userdata = std::string(), int length = -1) {
	INetworkStringTable *stringTable = g_Interfaces.NetworkStringTableContainerInstance->GetTable(tableIndex);

	if(NULL == stringTable) {
		throw InvalidStringTableExceptionType(tableIndex);
	}

#if SOURCE_ENGINE >= SE_ORANGEBOX
	stringTable->AddString(true, str.c_str(), length, userdata.c_str());
#else
	stringTable->AddString(str.c_str(), length, userdata.c_str());
#endif
}

PointContentsType sdktools__get_point_contents(VectorType position) {
	IHandleEntity *handleEntity;
	Vector positionVec(position.X, position.Y, position.Z);
	
#if SOURCE_ENGINE >= SE_LEFT4DEAD
	int mask = g_Interfaces.EngineTraceInstance->GetPointContents(positionVec, MASK_ALL, &handleEntity);
#else
	int mask = g_Interfaces.EngineTraceInstance->GetPointContents(positionVec, &handleEntity);
#endif
	
	CBaseEntity *entity = reinterpret_cast<CBaseEntity *>(handleEntity);

	int entityIndex = 0;

	if(entity != NULL) {
		edict_t *edict = g_Interfaces.ServerGameEntsInstance->BaseEntityToEdict(entity);

		if(edict != NULL && !edict->IsFree()) {
			entityIndex = IndexOfEdict(edict);
		}
	}

	return PointContentsType(entityIndex, mask);
}

PointContentsType sdktools__get_point_contents_entity(int entityIndex, VectorType position) {
	edict_t *edict = PEntityOfEntIndex(entityIndex);
	
	if(edict == NULL || edict->IsFree()) {
		throw InvalidEdictExceptionType(entityIndex);
	}

	CBaseEntity *entity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(edict);

	if(entity == NULL) {
		throw InvalidEntityExceptionType(entityIndex);
	}

	Vector positionVec(position.X, position.Y, position.Z);

	return PointContentsType(entityIndex, g_Interfaces.EngineTraceInstance->GetPointContents_Collideable(edict->GetCollideable(), positionVec));
}

TraceResultsType sdktools__trace_ray(VectorType startPosition, VectorType endPosition, unsigned int flags, py::object filter = BOOST_PY_NONE) {
	Ray_t ray;

	ray.Init(Vector(startPosition.X, startPosition.Y, startPosition.Z),
		Vector(endPosition.X, endPosition.Y, endPosition.Z));

	trace_t trace;

	PyThreadState *threadState = PyThreadState_Get();

	ViperTraceFilter traceFilter(threadState, filter);

	g_Interfaces.EngineTraceInstance->TraceRay(ray, flags, &traceFilter, &trace);

	int entityIndex = 0;

	CBaseEntity *entity = trace.m_pEnt;

	if(entity != NULL) {
		edict_t *edict = g_Interfaces.ServerGameEntsInstance->BaseEntityToEdict(entity);

		if(edict != NULL && !edict->IsFree()) {
			entityIndex = IndexOfEdict(edict);
		}
	}

	return TraceResultsType(trace.fraction, VectorType(trace.endpos.x, trace.endpos.y, trace.endpos.z), entityIndex, trace.DidHit(), trace.hitgroup,
		VectorType(trace.plane.normal.x, trace.plane.normal.y, trace.plane.normal.z));
}

TraceResultsType sdktools__trace_hull(VectorType startPosition, VectorType endPosition, VectorType mins, VectorType maxs, unsigned int flags, py::object filter = BOOST_PY_NONE) {
	Ray_t ray;

	ray.Init(Vector(startPosition.X, startPosition.Y, startPosition.Z),
		Vector(endPosition.X, endPosition.Y, endPosition.Z),
		Vector(mins.X, mins.Y, mins.Z),
		Vector(maxs.X, maxs.Y, maxs.Z));

	trace_t trace;

	PyThreadState *threadState = PyThreadState_Get();

	ViperTraceFilter traceFilter(threadState, filter);

	g_Interfaces.EngineTraceInstance->TraceRay(ray, flags, &traceFilter, &trace);

	int entityIndex = 0;

	CBaseEntity *entity = trace.m_pEnt;

	if(entity != NULL) {
		edict_t *edict = g_Interfaces.ServerGameEntsInstance->BaseEntityToEdict(entity);

		if(edict != NULL && !edict->IsFree()) {
			entityIndex = IndexOfEdict(edict);
		}
	}

	return TraceResultsType(trace.fraction, VectorType(trace.endpos.x, trace.endpos.y, trace.endpos.z), entityIndex, trace.DidHit(), trace.hitgroup,
		VectorType(trace.plane.normal.x, trace.plane.normal.y, trace.plane.normal.z));
}

VectorType sdktools__get_max_trace_point(VectorType position, VectorType angle) {
	Vector positionVec(position.X, position.Y, position.Z);
	Vector endVec;

	QAngle dirAngles(angle.X, angle.Y, angle.Z);
	
	AngleVectors(dirAngles, &endVec);

	endVec.NormalizeInPlace();
	endVec = positionVec + endVec * MAX_TRACE_LENGTH;

	return VectorType(endVec.x, endVec.y, endVec.z);
}

VectorType sdktools__get_client_eye_angles(int clientIndex) {
	if (NULL == sdktools__GetClientEyeAnglesCallWrapper) {
		int offset;
		void *addr;

		PassInfo pass[1];
		pass[0].type = PassType_Basic;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(QAngle *);
		
		if (g_Interfaces.GameConfigInstance->GetOffset("EyeAngles", &offset)) {
			sdktools__GetClientEyeAnglesCallWrapper = g_Interfaces.BinToolsInstance->CreateVCall(offset, 0, 0, &pass[0], NULL, 0);
		}
		else if(g_Interfaces.GameConfigInstance->GetMemSig("EyeAngles", &addr)) {
			sdktools__GetClientEyeAnglesCallWrapper = g_Interfaces.BinToolsInstance->CreateCall(addr, CallConv_ThisCall, &pass[0], NULL, 0);
		}
		else {
			throw SDKToolsModSupportNotAvailableExceptionType("EyeAngles");
		}
	}

	SourceMod::IGamePlayer *player = playerhelpers->GetGamePlayer(clientIndex);

	if(!player->IsConnected()) {
		throw ClientNotConnectedExceptionType(clientIndex);
	}

	if(!player->IsInGame()) {
		throw ClientNotInGameExceptionType(clientIndex);
	}
	
	CBaseEntity *entity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(player->GetEdict());

	unsigned char vstk[sizeof(CBaseEntity *)];
	unsigned char *vptr = vstk;

	*(CBaseEntity **)vptr = entity;

	QAngle *ret;

	sdktools__GetClientEyeAnglesCallWrapper->Execute(vstk, &ret);

	return VectorType(ret->x, ret->y, ret->z);
}

bool sdktools__is_point_outside_world(VectorType position) {
	return g_Interfaces.EngineTraceInstance->PointOutsideWorld(Vector(position.X, position.Y, position.Z));
}


bool sdktools__VoiceDecHookCount() {
	if (sdktools__VoiceHookCount == 0) {
		SH_REMOVE_HOOK(IVoiceServer, SetClientListening, g_Interfaces.VoiceServerInstance, SH_STATIC(&sdktools__OnSetClientListening), false);
		return true;
	}

	return false;
}

void sdktools__VoiceIncHookCount() {
	if (!sdktools__VoiceHookCount++) {
		SH_ADD_HOOK(IVoiceServer, SetClientListening, g_Interfaces.VoiceServerInstance, SH_STATIC(&sdktools__OnSetClientListening), false);
	}
}

void sdktools__OnClientCommand(edict_t *pEntity, const CCommand &args) {
	int client = IndexOfEdict(pEntity);

	if ((args.ArgC() > 1) && (stricmp(args.Arg(0), "vban") == 0))
	{
		for (int i = 1; (i < args.ArgC()) && (i < 3); i++)
		{
			unsigned long mask = 0;
			sscanf(args.Arg(i), "%p", (void**)&mask);
			
			for (int j = 0; j < 32; j++)
			{
				sdktools__ClientMutes[client][1 + j + 32 * (i - 1)] = !!(mask & 1 << j);
			}
		}
	}

	RETURN_META(MRES_IGNORED);
}

bool sdktools__OnSetClientListening(int iReceiver, int iSender, bool bListen)
{
	if (sdktools__ClientMutes[iReceiver][iSender])
	{
		RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, bListen, &IVoiceServer::SetClientListening, (iReceiver, iSender, false));
	}

	if (sdktools__VoiceFlags[iSender] & SPEAK_MUTED)
	{
		RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, bListen, &IVoiceServer::SetClientListening, (iReceiver, iSender, false));
	}

	if (sdktools__VoiceMap[iReceiver][iSender] == Listen_No)
	{
		RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, bListen, &IVoiceServer::SetClientListening, (iReceiver, iSender, false));
	}
	else if (sdktools__VoiceMap[iReceiver][iSender] == Listen_Yes)
	{
		RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, bListen, &IVoiceServer::SetClientListening, (iReceiver, iSender, true));
	}

	if ((sdktools__VoiceFlags[iSender] & SPEAK_ALL) || (sdktools__VoiceFlags[iReceiver] & SPEAK_LISTENALL))
	{
		RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, bListen, &IVoiceServer::SetClientListening, (iReceiver, iSender, true));
	}

	if ((sdktools__VoiceFlags[iSender] & SPEAK_TEAM) || (sdktools__VoiceFlags[iReceiver] & SPEAK_LISTENTEAM))
	{
		IGamePlayer *pReceiver = playerhelpers->GetGamePlayer(iReceiver);
		IGamePlayer *pSender = playerhelpers->GetGamePlayer(iSender);

		if (pReceiver && pSender && pReceiver->IsInGame() && pSender->IsInGame())
		{
			IPlayerInfo *pRInfo = pReceiver->GetPlayerInfo();
			IPlayerInfo *pSInfo = pSender->GetPlayerInfo();

			if (pRInfo && pSInfo && pRInfo->GetTeamIndex() == pSInfo->GetTeamIndex())
			{
				RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, bListen, &IVoiceServer::SetClientListening, (iReceiver, iSender, true));
			}
		}
	}

	RETURN_META_VALUE(MRES_IGNORED, bListen);
}

void sdktools__set_client_listening_flags(int clientIndex, size_t flags) {
	IGamePlayer *player = playerhelpers->GetGamePlayer(clientIndex);
	
	if(!player->IsConnected()) {
		throw ClientNotConnectedExceptionType(clientIndex);
	}

	if (!flags && sdktools__VoiceFlags[clientIndex])
	{
		sdktools__VoiceDecHookCount();
	}
	else if (!sdktools__VoiceFlags[clientIndex] && flags)
	{
		sdktools__VoiceIncHookCount();
	}

	sdktools__VoiceFlags[clientIndex] = flags;
}

size_t sdktools__get_client_listening_flags(int clientIndex) {
	IGamePlayer *player = playerhelpers->GetGamePlayer(clientIndex);
	
	if(!player->IsConnected()) {
		throw ClientNotConnectedExceptionType(clientIndex);
	}

	return sdktools__VoiceFlags[clientIndex];
}

void sdktools__set_listen_override(int listenerClientIndex, int senderClientIndex, ListenOverride overrideValue)
{
	int r, s;
	IGamePlayer *player;
	
	player = playerhelpers->GetGamePlayer(listenerClientIndex);
	
	if (!player->IsConnected())
	{
		throw ClientNotConnectedExceptionType(listenerClientIndex);
	}

	player = playerhelpers->GetGamePlayer(senderClientIndex);
	
	if (!player->IsConnected())
	{
		throw ClientNotConnectedExceptionType(senderClientIndex);
	}

	r = listenerClientIndex;
	s = senderClientIndex;
	
	if (sdktools__VoiceMap[r][s] == Listen_Default && overrideValue != Listen_Default)
	{
		sdktools__VoiceMap[r][s] = overrideValue;
		sdktools__VoiceIncHookCount();
	}
	else if (sdktools__VoiceMap[r][s] != Listen_Default && overrideValue == Listen_Default)
	{
		sdktools__VoiceMap[r][s] = overrideValue;
		sdktools__VoiceDecHookCount();
	}
	else
	{
		sdktools__VoiceMap[r][s] = overrideValue;
	}
}

ListenOverride sdktools__get_listen_override(int listenerClientIndex, int senderClientIndex)
{
	IGamePlayer *player;
	
	player = playerhelpers->GetGamePlayer(listenerClientIndex);
	
	if (!player->IsConnected())
	{
		throw ClientNotConnectedExceptionType(listenerClientIndex);
	}

	player = playerhelpers->GetGamePlayer(senderClientIndex);
	
	if (!player->IsConnected())
	{
		throw ClientNotConnectedExceptionType(senderClientIndex);
	}

	return sdktools__VoiceMap[listenerClientIndex][senderClientIndex];
}

bool sdktools__is_client_muted(int muterClientIndex, int muteeClientIndex) {
	IGamePlayer *player;
	
	player = playerhelpers->GetGamePlayer(muterClientIndex);
	
	if (!player->IsConnected())
	{
		throw ClientNotConnectedExceptionType(muterClientIndex);
	}

	player = playerhelpers->GetGamePlayer(muteeClientIndex);
	
	if (!player->IsConnected())
	{
		throw ClientNotConnectedExceptionType(muteeClientIndex);
	}

	return sdktools__ClientMutes[muterClientIndex][muteeClientIndex];
}

void *sdktools__FindTempEntByName(std::string name) {
	if(!sdktools__TELoaded) {
		sdktools__LoadTempEnts();
	}

	void *iter = sdktools__TEListHead;
	
	while (iter) {
		const char *actualName = *(const char **)((unsigned char *)iter + sdktools__TENameOffs);
			
		if (!actualName) {
			continue;
		}
			
		if (name == actualName) {
			return iter;
		}

		iter = *(void **)((unsigned char *)iter + sdktools__TENextOffs);
	}
	
	return NULL;
}

void sdktools__LoadTempEnts() {
	if(sdktools__TELoaded) {
		return;
	}

	void *addr = NULL;
	int offset;

	/* Read our sigs and offsets from the config file */
	if (g_Interfaces.GameConfigInstance->GetMemSig("s_pTempEntities", &addr) && addr) {
		sdktools__TEListHead = *(void **)addr;
	}
	else if (g_Interfaces.GameConfigInstance->GetMemSig("CBaseTempEntity", &addr) && addr) {
		if (!g_Interfaces.GameConfigInstance->GetOffset("s_pTempEntities", &offset)) {
			throw SDKToolsModSupportNotAvailableExceptionType("TempEnts");
		}

		sdktools__TEListHead = **(void ***)((unsigned char *)addr + offset);
	}
	else {
		throw SDKToolsModSupportNotAvailableExceptionType("TempEnts");
	}

	if (!g_Interfaces.GameConfigInstance->GetOffset("GetTEName", &sdktools__TENameOffs)) {
		throw SDKToolsModSupportNotAvailableExceptionType("TempEnts");
	}
	if (!g_Interfaces.GameConfigInstance->GetOffset("GetTENext", &sdktools__TENextOffs)) {
		throw SDKToolsModSupportNotAvailableExceptionType("TempEnts");
	}
	if (!g_Interfaces.GameConfigInstance->GetOffset("TE_GetServerClass", &sdktools__TEGetClassNameOffs)) {
		throw SDKToolsModSupportNotAvailableExceptionType("TempEnts");
	}

	/* Create the GetServerClass call */
	PassInfo retinfo;
	retinfo.flags = PASSFLAG_BYVAL;
	retinfo.type = PassType_Basic;
	retinfo.size = sizeof(ServerClass *);

	sdktools__TEGetServerClassCallWrapper = g_Interfaces.BinToolsInstance->CreateVCall(sdktools__TEGetClassNameOffs, 0, 0, &retinfo, NULL, 0);

	sdktools__TELoaded = true;
}

void sdktools__te_start(std::string name) {
	void *effect = sdktools__FindTempEntByName(name);

	if(effect == NULL) {
		throw InvalidTempEntExceptionType(name);
	}

	sdktools__TECurrentEffect = effect;

	sdktools__TEGetServerClassCallWrapper->Execute(&sdktools__TECurrentEffect, &sdktools__TECurrentServerClass);
}

int sdktools__FindPropOffset(std::string prop, int *size) {
	SendProp *sendPropInstance = gamehelpers->FindInSendTable(sdktools__TECurrentServerClass->GetName(), prop.c_str());

	if(sendPropInstance == NULL) {
		throw InvalidTempEntPropertyExceptionType(prop);
	}

	if(size) {
		*size = sendPropInstance->m_nBits;
	}

	return sendPropInstance->GetOffset();
}

bool sdktools__te_is_valid(std::string name) {
	return sdktools__FindTempEntByName(name) != NULL;
}

bool sdktools__te_is_valid_prop(std::string prop) {
	if(NULL == sdktools__TECurrentEffect) {
		throw NoTempEntCallInProgressExceptionType();
	}

	return gamehelpers->FindInSendTable(sdktools__TECurrentServerClass->GetName(), prop.c_str());
}

void sdktools__te_write_num(std::string prop, int num) {
	if(NULL == sdktools__TECurrentEffect) {
		throw NoTempEntCallInProgressExceptionType();
	}

	int size;
	int offset = sdktools__FindPropOffset(prop, &size);

	if (size <= 8) {
		*((uint8_t *)sdktools__TECurrentEffect + offset) = num;
	} else if (size <= 16) {
		*(short *)((uint8_t *)sdktools__TECurrentEffect + offset) = num;
	} else {
		*(int *)((uint8_t *)sdktools__TECurrentEffect + offset) = num;
	}
}

int sdktools__te_read_num(std::string prop) {
	if(NULL == sdktools__TECurrentEffect) {
		throw NoTempEntCallInProgressExceptionType();
	}

	int size;
	int offset = sdktools__FindPropOffset(prop, &size);

	if (size <= 8) {
		return *((uint8_t *)sdktools__TECurrentEffect + offset);
	} else if (size <= 16) {
		return *(short *)((uint8_t *)sdktools__TECurrentEffect + offset);
	} else {
		return *(int *)((uint8_t *)sdktools__TECurrentEffect + offset);
	}
}

void sdktools__te_write_float(std::string prop, float value) {
	if(NULL == sdktools__TECurrentEffect) {
		throw NoTempEntCallInProgressExceptionType();
	}

	int size;
	int offset = sdktools__FindPropOffset(prop, &size);

	*(float *)((uint8_t *)sdktools__TECurrentEffect + offset) = value;
}

float sdktools__te_read_float(std::string prop) {
	if(NULL == sdktools__TECurrentEffect) {
		throw NoTempEntCallInProgressExceptionType();
	}

	int size;
	int offset = sdktools__FindPropOffset(prop, &size);

	return *(float *)((uint8_t *)sdktools__TECurrentEffect + offset);
}

void sdktools__te_write_vector(std::string prop, VectorType vec) {
	if(NULL == sdktools__TECurrentEffect) {
		throw NoTempEntCallInProgressExceptionType();
	}

	int size;
	int offset = sdktools__FindPropOffset(prop, &size);

	Vector *v = (Vector*)((uint8_t *)sdktools__TECurrentEffect + offset);

	v->x = vec.X;
	v->y = vec.Y;
	v->z = vec.Z;
}

VectorType sdktools__te_read_vector(std::string prop) {
	if(NULL == sdktools__TECurrentEffect) {
		throw NoTempEntCallInProgressExceptionType();
	}

	int size;
	int offset = sdktools__FindPropOffset(prop, &size);

	Vector *v = (Vector*)((uint8_t *)sdktools__TECurrentEffect + offset);

	return VectorType(v->x, v->y, v->z);
}

void sdktools__te_send(py::list clientsList, float delay = 0.0f) {
	if(NULL == sdktools__TECurrentEffect) {
		throw NoTempEntCallInProgressExceptionType();
	}

	std::vector<int> clientsVec;

	int clientsListSize = py::len(clientsList);

	for(int clientIndex = 0; clientIndex < clientsListSize; clientIndex++) {
		clientsVec.push_back(py::extract<int>(clientsList[clientIndex]));
	}

	ViperRecipientFilter filter(clientsVec, false, false);

	engine->PlaybackTempEntity(filter, delay, sdktools__TECurrentEffect, sdktools__TECurrentServerClass->m_pTable, sdktools__TECurrentServerClass->m_ClassID);

	sdktools__TECurrentEffect = NULL;
}

int sdktools__get_game_rules_int(int offset) {
	if(!g_pSM->IsMapRunning()) {
		throw MapMustBeRunningExceptionType();
	}

	void *gameRules = g_Interfaces.SDKToolsInstance->GetGameRules();

	if(NULL == gameRules) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRules");
	}

	if (offset <= 0 || offset > 32768) {
		throw EntityOffsetOutOfRangeExceptionType(offset);
	}

	return *(int *)((uint8_t *)gameRules + offset);
}

unsigned int sdktools__get_game_rules_unsigned_int(int offset) {
	if(!g_pSM->IsMapRunning()) {
		throw MapMustBeRunningExceptionType();
	}

	void *gameRules = g_Interfaces.SDKToolsInstance->GetGameRules();

	if(NULL == gameRules) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRules");
	}

	if (offset <= 0 || offset > 32768) {
		throw EntityOffsetOutOfRangeExceptionType(offset);
	}

	return *(unsigned int *)((uint8_t *)gameRules + offset);
}

short sdktools__get_game_rules_short(int offset) {
	if(!g_pSM->IsMapRunning()) {
		throw MapMustBeRunningExceptionType();
	}

	void *gameRules = g_Interfaces.SDKToolsInstance->GetGameRules();

	if(NULL == gameRules) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRules");
	}

	if (offset <= 0 || offset > 32768) {
		throw EntityOffsetOutOfRangeExceptionType(offset);
	}

	return *(short *)((uint8_t *)gameRules + offset);
}

unsigned short sdktools__get_game_rules_unsigned_short(int offset) {
	if(!g_pSM->IsMapRunning()) {
		throw MapMustBeRunningExceptionType();
	}

	void *gameRules = g_Interfaces.SDKToolsInstance->GetGameRules();

	if(NULL == gameRules) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRules");
	}

	if (offset <= 0 || offset > 32768) {
		throw EntityOffsetOutOfRangeExceptionType(offset);
	}

	return *(unsigned short *)((uint8_t *)gameRules + offset);
}

char sdktools__get_game_rules_char(int offset) {
	if(!g_pSM->IsMapRunning()) {
		throw MapMustBeRunningExceptionType();
	}

	void *gameRules = g_Interfaces.SDKToolsInstance->GetGameRules();

	if(NULL == gameRules) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRules");
	}

	if (offset <= 0 || offset > 32768) {
		throw EntityOffsetOutOfRangeExceptionType(offset);
	}

	return *(char *)((uint8_t *)gameRules + offset);
}

unsigned char sdktools__get_game_rules_unsigned_char(int offset) {
	if(!g_pSM->IsMapRunning()) {
		throw MapMustBeRunningExceptionType();
	}

	void *gameRules = g_Interfaces.SDKToolsInstance->GetGameRules();

	if(NULL == gameRules) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRules");
	}

	if (offset <= 0 || offset > 32768) {
		throw EntityOffsetOutOfRangeExceptionType(offset);
	}

	return *(unsigned char *)((uint8_t *)gameRules + offset);
}

float sdktools__get_game_rules_float(int offset) {
	if(!g_pSM->IsMapRunning()) {
		throw MapMustBeRunningExceptionType();
	}

	void *gameRules = g_Interfaces.SDKToolsInstance->GetGameRules();

	if(NULL == gameRules) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRules");
	}

	if (offset <= 0 || offset > 32768) {
		throw EntityOffsetOutOfRangeExceptionType(offset);
	}

	return *(float *)((uint8_t *)gameRules + offset);
}

int sdktools__get_game_rules_entity(int offset) {
	if(!g_pSM->IsMapRunning()) {
		throw MapMustBeRunningExceptionType();
	}

	void *gameRules = g_Interfaces.SDKToolsInstance->GetGameRules();

	if(NULL == gameRules) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRules");
	}

	if (offset <= 0 || offset > 32768) {
		throw EntityOffsetOutOfRangeExceptionType(offset);
	}

	CBaseHandle &baseHandle = *(CBaseHandle *)((uint8_t *)gameRules + offset);

	return baseHandle.GetEntryIndex();
}

VectorType sdktools__get_game_rules_vector(int offset) {
	if(!g_pSM->IsMapRunning()) {
		throw MapMustBeRunningExceptionType();
	}

	void *gameRules = g_Interfaces.SDKToolsInstance->GetGameRules();

	if(NULL == gameRules) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRules");
	}

	if (offset <= 0 || offset > 32768) {
		throw EntityOffsetOutOfRangeExceptionType(offset);
	}

	Vector *v = (Vector *)((uint8_t *)gameRules + offset);

	return VectorType(v->x, v->y, v->z);
}

std::string sdktools__get_game_rules_string(int offset) {
	if(!g_pSM->IsMapRunning()) {
		throw MapMustBeRunningExceptionType();
	}

	void *gameRules = g_Interfaces.SDKToolsInstance->GetGameRules();

	if(NULL == gameRules) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRules");
	}

	if (offset <= 0 || offset > 32768) {
		throw EntityOffsetOutOfRangeExceptionType(offset);
	}

	char *str = (char *)((uint8_t *)gameRules + offset);

	return std::string(str);
}

static CBaseEntity *FindEntityByNetClass(int start, const char *classname)
{
	int maxEntities = g_Interfaces.GlobalVarsInstance->maxEntities;

	for (int i = start; i < maxEntities; i++)
	{
		edict_t *current = gamehelpers->EdictOfIndex(i);
		if (current == NULL)
			continue;

		IServerNetworkable *network = current->GetNetworkable();
		if (network == NULL)
			continue;

		ServerClass *sClass = network->GetServerClass();
		const char *name = sClass->GetName();

		if (!strcmp(name, classname))
			return gamehelpers->ReferenceToEntity(gamehelpers->IndexOfEdict(current));		
	}

	return NULL;
}

static CBaseEntity* GetGameRulesProxyEnt()
{
	static cell_t proxyEntRef = -1;
	CBaseEntity *pProxy;
	if (proxyEntRef == -1 || (pProxy = gamehelpers->ReferenceToEntity(proxyEntRef)) == NULL)
	{
		pProxy = FindEntityByNetClass(playerhelpers->GetMaxClients(), sdktools__GameRulesProxyClassName.c_str());
		proxyEntRef = gamehelpers->EntityToReference(pProxy);
	}
	
	return pProxy;
}

void sdktools__set_game_rules_int(int offset, int newValue, bool changeState = false) {
	if(!g_pSM->IsMapRunning()) {
		throw MapMustBeRunningExceptionType();
	}

	void *gameRules = g_Interfaces.SDKToolsInstance->GetGameRules();

	if(NULL == gameRules) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRules");
	}

	if (offset <= 0 || offset > 32768) {
		throw EntityOffsetOutOfRangeExceptionType(offset);
	}

	CBaseEntity *gameRulesProxy = NULL;
	if (changeState && ((gameRulesProxy = GetGameRulesProxyEnt()) == NULL)) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRulesProxy");
	}
	
	*(int *)((uint8_t *)gameRules + offset) = newValue;
	
	if (changeState) {
		*(int *)((uint8_t *)gameRulesProxy + offset) = newValue;
		gamehelpers->SetEdictStateChanged(gamehelpers->EdictOfIndex(gamehelpers->EntityToBCompatRef(gameRulesProxy)), offset);
	}
}

void sdktools__set_game_rules_unsigned_int(int offset, unsigned int newValue, bool changeState = false) {
	if(!g_pSM->IsMapRunning()) {
		throw MapMustBeRunningExceptionType();
	}

	void *gameRules = g_Interfaces.SDKToolsInstance->GetGameRules();

	if(NULL == gameRules) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRules");
	}

	if (offset <= 0 || offset > 32768) {
		throw EntityOffsetOutOfRangeExceptionType(offset);
	}

	CBaseEntity *gameRulesProxy = NULL;
	if (changeState && ((gameRulesProxy = GetGameRulesProxyEnt()) == NULL)) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRulesProxy");
	}
	
	*(unsigned int *)((uint8_t *)gameRules + offset) = newValue;
	
	if (changeState) {
		*(unsigned int *)((uint8_t *)gameRulesProxy + offset) = newValue;
		gamehelpers->SetEdictStateChanged(gamehelpers->EdictOfIndex(gamehelpers->EntityToBCompatRef(gameRulesProxy)), offset);
	}
}

void sdktools__set_game_rules_short(int offset, short newValue, bool changeState = false) {
	if(!g_pSM->IsMapRunning()) {
		throw MapMustBeRunningExceptionType();
	}

	void *gameRules = g_Interfaces.SDKToolsInstance->GetGameRules();

	if(NULL == gameRules) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRules");
	}

	if (offset <= 0 || offset > 32768) {
		throw EntityOffsetOutOfRangeExceptionType(offset);
	}

	CBaseEntity *gameRulesProxy = NULL;
	if (changeState && ((gameRulesProxy = GetGameRulesProxyEnt()) == NULL)) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRulesProxy");
	}
	
	*(short *)((uint8_t *)gameRules + offset) = newValue;
	
	if (changeState) {
		*(short *)((uint8_t *)gameRulesProxy + offset) = newValue;
		gamehelpers->SetEdictStateChanged(gamehelpers->EdictOfIndex(gamehelpers->EntityToBCompatRef(gameRulesProxy)), offset);
	}
}

void sdktools__set_game_rules_unsigned_short(int offset, unsigned short newValue, bool changeState = false) {
	if(!g_pSM->IsMapRunning()) {
		throw MapMustBeRunningExceptionType();
	}

	void *gameRules = g_Interfaces.SDKToolsInstance->GetGameRules();

	if(NULL == gameRules) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRules");
	}

	if (offset <= 0 || offset > 32768) {
		throw EntityOffsetOutOfRangeExceptionType(offset);
	}

	CBaseEntity *gameRulesProxy = NULL;
	if (changeState && ((gameRulesProxy = GetGameRulesProxyEnt()) == NULL)) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRulesProxy");
	}
	
	*(unsigned short *)((uint8_t *)gameRules + offset) = newValue;
	
	if (changeState) {
		*(unsigned short *)((uint8_t *)gameRulesProxy + offset) = newValue;
		gamehelpers->SetEdictStateChanged(gamehelpers->EdictOfIndex(gamehelpers->EntityToBCompatRef(gameRulesProxy)), offset);
	}
}

void sdktools__set_game_rules_char(int offset, char newValue, bool changeState = false) {
	if(!g_pSM->IsMapRunning()) {
		throw MapMustBeRunningExceptionType();
	}

	void *gameRules = g_Interfaces.SDKToolsInstance->GetGameRules();

	if(NULL == gameRules) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRules");
	}

	if (offset <= 0 || offset > 32768) {
		throw EntityOffsetOutOfRangeExceptionType(offset);
	}

	CBaseEntity *gameRulesProxy = NULL;
	if (changeState && ((gameRulesProxy = GetGameRulesProxyEnt()) == NULL)) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRulesProxy");
	}
	
	*(char *)((uint8_t *)gameRules + offset) = newValue;
	
	if (changeState) {
		*(char *)((uint8_t *)gameRulesProxy + offset) = newValue;
		gamehelpers->SetEdictStateChanged(gamehelpers->EdictOfIndex(gamehelpers->EntityToBCompatRef(gameRulesProxy)), offset);
	}
}

void sdktools__set_game_rules_unsigned_char(int offset, unsigned char newValue, bool changeState = false) {
	if(!g_pSM->IsMapRunning()) {
		throw MapMustBeRunningExceptionType();
	}

	void *gameRules = g_Interfaces.SDKToolsInstance->GetGameRules();

	if(NULL == gameRules) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRules");
	}

	if (offset <= 0 || offset > 32768) {
		throw EntityOffsetOutOfRangeExceptionType(offset);
	}

	CBaseEntity *gameRulesProxy = NULL;
	if (changeState && ((gameRulesProxy = GetGameRulesProxyEnt()) == NULL)) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRulesProxy");
	}
	
	*(unsigned char *)((uint8_t *)gameRules + offset) = newValue;
	
	if (changeState) {
		*(unsigned char *)((uint8_t *)gameRulesProxy + offset) = newValue;
		gamehelpers->SetEdictStateChanged(gamehelpers->EdictOfIndex(gamehelpers->EntityToBCompatRef(gameRulesProxy)), offset);
	}
}

void sdktools__set_game_rules_float(int offset, float newValue, bool changeState = false) {
	if(!g_pSM->IsMapRunning()) {
		throw MapMustBeRunningExceptionType();
	}

	void *gameRules = g_Interfaces.SDKToolsInstance->GetGameRules();

	if(NULL == gameRules) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRules");
	}

	if (offset <= 0 || offset > 32768) {
		throw EntityOffsetOutOfRangeExceptionType(offset);
	}

	CBaseEntity *gameRulesProxy = NULL;
	if (changeState && ((gameRulesProxy = GetGameRulesProxyEnt()) == NULL)) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRulesProxy");
	}
	
	*(float *)((uint8_t *)gameRules + offset) = newValue;
	
	if (changeState) {
		*(float *)((uint8_t *)gameRulesProxy + offset) = newValue;
		gamehelpers->SetEdictStateChanged(gamehelpers->EdictOfIndex(gamehelpers->EntityToBCompatRef(gameRulesProxy)), offset);
	}
}

void sdktools__set_game_rules_entity(int offset, int newValue, bool changeState = false) {
	if(!g_pSM->IsMapRunning()) {
		throw MapMustBeRunningExceptionType();
	}

	void *gameRules = g_Interfaces.SDKToolsInstance->GetGameRules();

	if(NULL == gameRules) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRules");
	}

	if (offset <= 0 || offset > 32768) {
		throw EntityOffsetOutOfRangeExceptionType(offset);
	}

	CBaseEntity *gameRulesProxy = NULL;
	if (changeState && ((gameRulesProxy = GetGameRulesProxyEnt()) == NULL)) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRulesProxy");
	}

	CBaseHandle &baseHandle = *(CBaseHandle *)((uint8_t *)gameRules + offset);

	if(newValue == INVALID_EHANDLE_INDEX) {
		baseHandle.Set(NULL);
	}
	else {
		edict_t *otherEdict = PEntityOfEntIndex(newValue);

		if(otherEdict == NULL || otherEdict->IsFree()) {
			throw InvalidEdictExceptionType(newValue);
		}
	
		CBaseEntity *otherBaseEntity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(otherEdict);

		if(NULL == otherBaseEntity) {
			throw InvalidEntityExceptionType(newValue);
		}

		IHandleEntity *handleEntity = (IHandleEntity *)otherBaseEntity;

		baseHandle.Set(handleEntity);
	}
	
	if (changeState) {
		CBaseHandle &baseHandleProxy = *(CBaseHandle *)((uint8_t *)gameRulesProxy + offset);

		if(newValue == INVALID_EHANDLE_INDEX) {
			baseHandleProxy.Set(NULL);
		}
		else {
			edict_t *otherEdict = PEntityOfEntIndex(newValue);

			if(otherEdict == NULL || otherEdict->IsFree()) {
				throw InvalidEdictExceptionType(newValue);
			}
	
			CBaseEntity *otherBaseEntity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(otherEdict);

			if(NULL == otherBaseEntity) {
				throw InvalidEntityExceptionType(newValue);
			}

			IHandleEntity *handleEntity = (IHandleEntity *)otherBaseEntity;

			baseHandleProxy.Set(handleEntity);

			gamehelpers->SetEdictStateChanged(gamehelpers->EdictOfIndex(gamehelpers->EntityToBCompatRef(gameRulesProxy)), offset);
		}
	}
}

void sdktools__set_game_rules_vector(int offset, VectorType newValue, bool changeState = false) {
	if(!g_pSM->IsMapRunning()) {
		throw MapMustBeRunningExceptionType();
	}

	void *gameRules = g_Interfaces.SDKToolsInstance->GetGameRules();

	if(NULL == gameRules) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRules");
	}

	if (offset <= 0 || offset > 32768) {
		throw EntityOffsetOutOfRangeExceptionType(offset);
	}

	CBaseEntity *gameRulesProxy = NULL;
	if (changeState && ((gameRulesProxy = GetGameRulesProxyEnt()) == NULL)) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRulesProxy");
	}
	
	Vector *v = (Vector *)((uint8_t *)gameRules + offset);

	v->x = newValue.X;
	v->y = newValue.Y;
	v->z = newValue.Z;
	
	if (changeState) {
		Vector *vProxy = (Vector *)((uint8_t *)gameRulesProxy + offset);

		vProxy->x = newValue.X;
		vProxy->y = newValue.Y;
		vProxy->z = newValue.Z;

		gamehelpers->SetEdictStateChanged(gamehelpers->EdictOfIndex(gamehelpers->EntityToBCompatRef(gameRulesProxy)), offset);
	}
}

void sdktools__set_game_rules_string(int offset, std::string newValue, int maxLength, bool changeState = false) {
	if(!g_pSM->IsMapRunning()) {
		throw MapMustBeRunningExceptionType();
	}

	void *gameRules = g_Interfaces.SDKToolsInstance->GetGameRules();

	if(NULL == gameRules) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRules");
	}

	if (offset <= 0 || offset > 32768) {
		throw EntityOffsetOutOfRangeExceptionType(offset);
	}

	CBaseEntity *gameRulesProxy = NULL;
	if (changeState && ((gameRulesProxy = GetGameRulesProxyEnt()) == NULL)) {
		throw SDKToolsModSupportNotAvailableExceptionType("GameRulesProxy");
	}
	
	char *str = (char *)((uint8_t *)gameRules + offset);

	UTIL_Format(str, maxLength, "%s", newValue.c_str());

	if (changeState) {
		str = (char *)((uint8_t *)gameRulesProxy + offset);

		UTIL_Format(str, maxLength, "%s", newValue.c_str());

		gamehelpers->SetEdictStateChanged(gamehelpers->EdictOfIndex(gamehelpers->EntityToBCompatRef(gameRulesProxy)), offset);
	}
}

std::string sdktools__TEGetNameFromThisPtr(void *thisPtr) {
	return std::string(*(const char **)((unsigned char *)thisPtr + sdktools__TENameOffs));
}

void sdktools__OnPlaybackTempEntity(IRecipientFilter &filter, float delay, const void *pSender, const SendTable *pST, int classID) {
	std::string name = sdktools__TEGetNameFromThisPtr(const_cast<void *>(pSender));

	py::list clientsList;

	int clientCount = filter.GetRecipientCount();

	for(int c = 0; c < clientCount; c++) {
		clientsList.append<int>(filter.GetRecipientIndex(c));
	}

	std::list<TempEntHook> teHooksCopy = sdktools__TEHooks;

	for(std::list<TempEntHook>::iterator it = teHooksCopy.begin(); it != teHooksCopy.end(); it++) {
		TempEntHook hookInfo = *it;

		if(hookInfo.EffectName != name) {
			continue;
		}

		PyThreadState *currentThreadState = PyThreadState_Get();

		PyThreadState_Swap(hookInfo.ThreadState);

		SourceMod::ResultType resultType = SourceMod::Pl_Continue;

		void *oldEffect = sdktools__TECurrentEffect;

		sdktools__TECurrentEffect = const_cast<void *>(pSender);

		try {
			py::extract<SourceMod::ResultType>(hookInfo.PythonCallback(name, clientsList, delay));
		}
		catch(const py::error_already_set &) {
			PyErr_Print();
		}

		PyThreadState_Swap(currentThreadState);

		sdktools__TECurrentEffect = oldEffect;

		if(resultType != SourceMod::Pl_Continue) {
			RETURN_META(MRES_SUPERCEDE);
		}
	}

	RETURN_META(MRES_IGNORED);
}

void sdktools__add_temp_ent_hook(std::string effectName, py::object pythonCallback) {
	PyThreadState *threadState = PyThreadState_Get();

	if(sdktools__FindTempEntByName(effectName) == NULL) {
		throw InvalidTempEntExceptionType(effectName);
	}

	for(std::list<TempEntHook>::iterator it = sdktools__TEHooks.begin(); it != sdktools__TEHooks.end(); it++) {
		TempEntHook hookInfo = *it;

		if(hookInfo.EffectName != effectName || hookInfo.PythonCallback != pythonCallback) {
			continue;
		}

		return;
	}

	// otherwise make a new one
	sdktools__TEHooks.push_back(TempEntHook(effectName, pythonCallback, threadState));
}

void sdktools__remove_temp_ent_hook(std::string effectName, py::object pythonCallback) {
	PyThreadState *threadState = PyThreadState_Get();

	if(sdktools__FindTempEntByName(effectName) == NULL) {
		throw InvalidTempEntExceptionType(effectName);
	}


	for(std::list<TempEntHook>::iterator it = sdktools__TEHooks.begin(); it != sdktools__TEHooks.end(); it++) {
		TempEntHook hookInfo = *it;

		if(hookInfo.EffectName != effectName || hookInfo.PythonCallback != pythonCallback) {
			continue;
		}

		sdktools__TEHooks.erase(it);
		return;
	}

	throw TempEntHookDoesNotExistExceptionType(effectName);
}

void sdktools__hook_entity_output(std::string className, std::string output, py::object pythonCallback) {
	PyThreadState *threadState = PyThreadState_Get();

	for(std::list<EntityOutputClassNameHook>::iterator it = sdktools__EntityOutputClassNameHooks.begin(); it != sdktools__EntityOutputClassNameHooks.end(); it++) {
		EntityOutputClassNameHook hookInfo = *it;

		if(hookInfo.ClassName != className || hookInfo.Output != output || hookInfo.CallbackFunction != pythonCallback) {
			continue;
		}

		return;
	}

	sdktools__EntityOutputClassNameHooks.push_back(EntityOutputClassNameHook(className, output, threadState, pythonCallback));
}

void sdktools__unhook_entity_output(std::string className, std::string output, py::object pythonCallback) {
	PyThreadState *threadState = PyThreadState_Get();

	for(std::list<EntityOutputClassNameHook>::iterator it = sdktools__EntityOutputClassNameHooks.begin(); it != sdktools__EntityOutputClassNameHooks.end(); it++) {
		EntityOutputClassNameHook hookInfo = *it;

		if(hookInfo.ClassName != className || hookInfo.Output != output || hookInfo.CallbackFunction != pythonCallback) {
			continue;
		}

		sdktools__EntityOutputClassNameHooks.erase(it);
		return;
	}

	throw EntityOutputClassNameHookDoesNotExistExceptionType(className, output);
}

static const char *GetEntityClassname(CBaseEntity *pEntity)
{
	static int offset = -1;
	if (offset == -1)
	{
		datamap_t *pMap = gamehelpers->GetDataMap(pEntity);
		typedescription_t *pDesc = gamehelpers->FindInDataMap(pMap, "m_iClassname");
		offset = GetTypeDescOffs(pDesc);
	}

	return *(const char **)(((unsigned char *)pEntity) + offset);
}

static const char *FindOutputName(void *pOutput, CBaseEntity *pCaller)
{
	datamap_t *pMap = gamehelpers->GetDataMap(pCaller);

	while (pMap)
	{
		for (int i=0; i<pMap->dataNumFields; i++)
		{
			if (pMap->dataDesc[i].flags & FTYPEDESC_OUTPUT)
			{
				if ((char *)pCaller + GetTypeDescOffs(&pMap->dataDesc[i]) == pOutput)
				{
					return pMap->dataDesc[i].externalName;
				}
			}
		}
		pMap = pMap->baseMap;
	}

	return NULL;
}

void sdktools__OnFireOutput(void *pOutput, CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay) {
	if(sdktools__EntityOutputClassNameHooks.empty() || !pCaller) {
		return;
	}

	const char *classNameStr = GetEntityClassname(pCaller);
	const char *outputName = FindOutputName(pOutput, pCaller);

	edict_t *callerEdict = g_Interfaces.ServerGameEntsInstance->BaseEntityToEdict(pCaller);

	if(!callerEdict && callerEdict->IsFree()) {
		return;
	}

	int callerIndex = IndexOfEdict(callerEdict);

	int activatorIndex = -1;

	if(pActivator) {
		edict_t *activatorEdict = g_Interfaces.ServerGameEntsInstance->BaseEntityToEdict(pActivator);

		if(activatorEdict && !activatorEdict->IsFree()) {
			activatorIndex = IndexOfEdict(activatorEdict);
		}
	}

	if(!classNameStr || !outputName) {
		return;
	}

	std::string className = std::string(classNameStr);
	std::string output = std::string(outputName);

	std::list<EntityOutputClassNameHook> hooksCopy = sdktools__EntityOutputClassNameHooks;

	for(std::list<EntityOutputClassNameHook>::iterator it = hooksCopy.begin(); it != hooksCopy.end(); it++) {
		EntityOutputClassNameHook hookInfo = *it;

		if(hookInfo.ClassName != className || hookInfo.Output != output) {
			continue;
		}

		PyThreadState *currentThreadState = PyThreadState_Get();

		PyThreadState_Swap(hookInfo.ThreadState);

		try {
			py::extract<SourceMod::ResultType>(hookInfo.CallbackFunction(output, activatorIndex, callerIndex, fDelay));
		}
		catch(const py::error_already_set &) {
			PyErr_Print();
		}

		PyThreadState_Swap(currentThreadState);
	}
}

void sdktools__prefetch_sound(std::string name) {
	g_Interfaces.EngineSoundInstance->PrefetchSound(name.c_str());
}

float sdktools__get_sound_duration(std::string name) {
	return g_Interfaces.EngineSoundInstance->GetSoundDuration(name.c_str());
}

void sdktools__emit_ambient_sound(std::string name, VectorType position, int entityIndex = 0, int level = 75, int flags = 0, float volume = 1.0f, int pitch = 100, float delay = 0.0f) {
	engine->EmitAmbientSound(entityIndex, Vector(position.X, position.Y, position.Z), name.c_str(), volume, (soundlevel_t)level, flags, pitch, delay);
}

void sdktools__fade_client_volume(int clientIndex, float percent, float outTime, float holdTime, float inTime)
{
	IGamePlayer *player = playerhelpers->GetGamePlayer(clientIndex);
	
	if (!player->IsConnected()) {
		throw ClientNotConnectedExceptionType(clientIndex);
	}

	if (!player->IsInGame()) {
		throw ClientNotInGameExceptionType(clientIndex);
	}

	engine->FadeClientVolume(player->GetEdict(), percent, outTime, holdTime, inTime);
}

void sdktools__stop_sound(int entityIndex, int channel, std::string name) {
	g_Interfaces.EngineSoundInstance->StopSound(entityIndex, channel, name.c_str());
}

void sdktools__emit_sound(py::list clientsList, std::string sample, int entityIndex = 0, int channel = 0, int level = 75, int flags = 0, float volume = 1.0f, int pitch = 100, int speakerEntityIndex = -1, py::object origin = BOOST_PY_NONE, py::object direction = BOOST_PY_NONE, bool updatePosition = true, float soundTime = 0.0f, py::list additionalOrigins = py::list()) {
	int clientCount = py::len(clientsList);

	std::vector<int> clientsVec;

	for (int i = 0; i < clientCount; i++) {
		int clientIndex = py::extract<int>(clientsList[i]);

		IGamePlayer *player = playerhelpers->GetGamePlayer(clientIndex);

		if (!player->IsConnected()) {
			throw ClientNotConnectedExceptionType(clientIndex);
		}
		
		if(!player->IsInGame()) {
			throw ClientNotInGameExceptionType(clientIndex);
		}

		clientsVec.push_back(clientIndex);
	}

	ViperRecipientFilter recipientFilter(clientsVec, false, false);
	
	Vector *pOrigin = NULL;

	if(!origin.is_none()) {
		VectorType originVT = py::extract<VectorType>(origin);

		pOrigin = new Vector(originVT.X, originVT.Y, originVT.Z);
	}

	Vector *pDir = NULL;

	if(!direction.is_none()) {
		VectorType directionVT = py::extract<VectorType>(direction);

		pDir = new Vector(directionVT.X, directionVT.Y, directionVT.Z);
	}

	CUtlVector<Vector> *pOrigVec = NULL;

	int aoCount = py::len(additionalOrigins);

	if (aoCount > 0) {
		pOrigVec = new CUtlVector<Vector>;

		for (int i = 0; i < aoCount; i++) {
			VectorType vt = py::extract<VectorType>(additionalOrigins[i]);
			
			Vector vec(vt.X, vt.Y, vt.Z);

			pOrigVec->AddToTail(vec);
		}
	}

#if SOURCE_ENGINE == SE_ORANGEBOXVALVE
	g_Interfaces.EngineSoundInstance->EmitSound(recipientFilter, entityIndex, channel, sample.c_str(), volume, (soundlevel_t)level, flags, pitch, 0, pOrigin, pDir, pOrigVec, updatePosition, soundTime, speakerEntityIndex);
#else
	g_Interfaces.EngineSoundInstance->EmitSound(recipientFilter, entityIndex, channel, sample.c_str(), volume, (soundlevel_t)level, flags, pitch, pOrigin, pDir, pOrigVec, updatePosition, soundTime, speakerEntityIndex);
#endif

	if(pOrigin)
		delete pOrigin;
	if(pDir)
		delete pDir;
	if(pOrigVec)
		delete pOrigVec;
}

void sdktools__emit_sentence(py::list clientsList, int sentence, int entityIndex, int channel = 0, int level = 75, int flags = 0, float volume = 1.0f, int pitch = 100, int speakerEntityIndex = -1, py::object origin = BOOST_PY_NONE, py::object direction = BOOST_PY_NONE, bool updatePosition = true, float soundTime = 0.0f, py::list additionalOrigins = py::list()) {
	int clientCount = py::len(clientsList);

	std::vector<int> clientsVec;

	for (int i = 0; i < clientCount; i++) {
		int clientIndex = py::extract<int>(clientsList[i]);

		IGamePlayer *player = playerhelpers->GetGamePlayer(clientIndex);

		if (!player->IsConnected()) {
			throw ClientNotConnectedExceptionType(clientIndex);
		}
		
		if(!player->IsInGame()) {
			throw ClientNotInGameExceptionType(clientIndex);
		}

		clientsVec.push_back(clientIndex);
	}

	ViperRecipientFilter recipientFilter(clientsVec, false, false);
	
	Vector *pOrigin = NULL;

	if(!origin.is_none()) {
		VectorType originVT = py::extract<VectorType>(origin);

		pOrigin = new Vector(originVT.X, originVT.Y, originVT.Z);
	}

	Vector *pDir = NULL;

	if(!direction.is_none()) {
		VectorType directionVT = py::extract<VectorType>(direction);

		pDir = new Vector(directionVT.X, directionVT.Y, directionVT.Z);
	}

	CUtlVector<Vector> *pOrigVec = NULL;

	int aoCount = py::len(additionalOrigins);

	if (aoCount > 0) {
		pOrigVec = new CUtlVector<Vector>;

		for (int i = 0; i < aoCount; i++) {
			VectorType vt = py::extract<VectorType>(additionalOrigins[i]);
			
			Vector vec(vt.X, vt.Y, vt.Z);

			pOrigVec->AddToTail(vec);
		}
	}

	g_Interfaces.EngineSoundInstance->EmitSentenceByIndex(recipientFilter, entityIndex, channel, sentence, volume, (soundlevel_t)level, flags, pitch, 
#if SOURCE_ENGINE == SE_ORANGEBOXVALVE
		0, 
#endif
		pOrigin, pDir, pOrigVec, updatePosition, soundTime, speakerEntityIndex);

	if(pOrigin)
		delete pOrigin;
	if(pDir)
		delete pDir;
	if(pOrigVec)
		delete pOrigVec;
}

float sdktools__get_dist_gain_from_sound_level(int decibel, float distance) {
	return g_Interfaces.EngineSoundInstance->GetDistGainFromSoundLevel((soundlevel_t)decibel, distance);
}

py::list sdktools__find_entities_by_class_name(std::string className) {
	py::list returnList;

	if(className == "world") {
		returnList.append<int>(0);
		return returnList;
	}

	for(int entityIndex = 1; entityIndex <= 4096; entityIndex++) {
		edict_t *edict = PEntityOfEntIndex(entityIndex);

		if(!edict || edict->IsFree()) {
			continue;
		}

		CBaseEntity *entity = g_Interfaces.ServerGameEntsInstance->EdictToBaseEntity(edict);

		if(!entity) {
			continue;
		}

		if(className != GetEntityClassname(entity)) {
			continue;
		}

		returnList.append<int>(entityIndex);
	}

	return returnList;
}

bool FindTeamEntities(SendTable *pTable, const char *name)
{
	if (strcmp(pTable->GetName(), name) == 0)
	{
		return true;
	}

	int props = pTable->GetNumProps();
	SendProp *prop;

	for (int i=0; i<props; i++)
	{
		prop = pTable->GetProp(i);
		if (prop->GetDataTable())
		{
			if (FindTeamEntities(prop->GetDataTable(), name))
			{
				return true;
			}
		}
	}

	return false;
}

void sdktools__OnCoreMapStart(edict_t *pEdictList, int edictCount, int clientMax) {
	sdktools__Teams.clear();
	sdktools__Teams.resize(1);

	for (int i=0; i<edictCount; i++)
	{
		edict_t *pEdict = PEntityOfEntIndex(i);
		if (!pEdict || pEdict->IsFree())
		{
			continue;
		}
		if (!pEdict->GetNetworkable())
		{
			continue;
		}

		ServerClass *pClass = pEdict->GetNetworkable()->GetServerClass();
		if (FindTeamEntities(pClass->m_pTable, "DT_Team"))
		{
			SendProp *pTeamNumProp = gamehelpers->FindInSendTable(pClass->GetName(), "m_iTeamNum");

			if (pTeamNumProp != NULL)
			{
				int offset = pTeamNumProp->GetOffset();
				CBaseEntity *pEnt = pEdict->GetUnknown()->GetBaseEntity();
				int TeamIndex = *(int *)((unsigned char *)pEnt + offset);

				if (TeamIndex >= (int)sdktools__Teams.size())
				{
					sdktools__Teams.resize(TeamIndex+1);
				}
				sdktools__Teams[TeamIndex].ClassName = pClass->GetName();
				sdktools__Teams[TeamIndex].pEnt = pEnt;
			}
		}
	}
}


int sdktools__get_team_count() {
	return sdktools__Teams.size();
}

static int sdktools__TeamNameOffset = -1;

std::string sdktools__get_team_name(int team)
{
	if (size_t(team) >= sdktools__Teams.size()) {
		throw InvalidTeamExceptionType(team);
	}

	if (sdktools__TeamNameOffset == -1)
	{
		SendProp *prop = gamehelpers->FindInSendTable(sdktools__Teams[team].ClassName, "m_szTeamname");
		if (!prop) {
			throw SDKToolsModSupportNotAvailableExceptionType("TeamName");
		}

		sdktools__TeamNameOffset = prop->GetOffset();
	}

	const char *pName = (const char *)((unsigned char *)sdktools__Teams[team].pEnt + sdktools__TeamNameOffset);

	if(!pName) {
		throw InvalidTeamExceptionType(team); 
	}

	return std::string(pName);
}

int sdktools__get_team_score(int teamindex) {
	if (teamindex >= (int)sdktools__Teams.size() || !sdktools__Teams[teamindex].ClassName) {
		throw InvalidTeamExceptionType(teamindex);
	}

	static int offset = gamehelpers->FindInSendTable(sdktools__Teams[teamindex].ClassName, "m_iScore")->GetOffset();

	return *(int *)((unsigned char *)sdktools__Teams[teamindex].pEnt + offset);
}

void sdktools__set_team_score(int teamindex, int newScore) {
	if (teamindex >= (int)sdktools__Teams.size() || !sdktools__Teams[teamindex].ClassName) {
		throw InvalidTeamExceptionType(teamindex);
	}

	static int offset = gamehelpers->FindInSendTable(sdktools__Teams[teamindex].ClassName, "m_iScore")->GetOffset();

	CBaseEntity *pTeam = sdktools__Teams[teamindex].pEnt;
	*(int *)((unsigned char *)pTeam + offset) = newScore;

	edict_t *pEdict = g_Interfaces.ServerGameEntsInstance->BaseEntityToEdict(pTeam);
	gamehelpers->SetEdictStateChanged(pEdict, offset);
}

int sdktools__get_team_client_count(int teamindex) {
	if (teamindex >= (int)sdktools__Teams.size() || !sdktools__Teams[teamindex].ClassName) {
		throw InvalidTeamExceptionType(teamindex);
	}

	SendProp *pProp = gamehelpers->FindInSendTable(sdktools__Teams[teamindex].ClassName, "\"player_array\"");
	ArrayLengthSendProxyFn fn = pProp->GetArrayLengthProxy();

	return fn(sdktools__Teams[teamindex].pEnt, 0);
}

NetStatsType sdktools__get_server_net_stats() {
	IServer *server = g_Interfaces.SDKToolsInstance->GetIServer();

	if (NULL == server) {
		throw IServerNotFoundExceptionType();
	}

	float in, out;

	server->GetNetStats(in, out);

	return NetStatsType(in, out);
}

void sdktools__set_client_info(int clientIndex, std::string key, std::string value) {
	IServer *server = g_Interfaces.SDKToolsInstance->GetIServer();

	if (NULL == server) {
		throw IServerNotFoundExceptionType();
	}

	if (NULL == sdktools__SetUserCvarCallWrapper) {
		int offset;
		void *addr;

		PassInfo pass[2];
		pass[0].type = PassType_Basic;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(const char *);
		pass[1].type = PassType_Basic;
		pass[1].flags = PASSFLAG_BYVAL;
		pass[1].size = sizeof(const char *);
		
		if (g_Interfaces.GameConfigInstance->GetOffset("SetUserCvar", &offset)) {
			sdktools__SetUserCvarCallWrapper = g_Interfaces.BinToolsInstance->CreateVCall(offset, 0, 0, NULL, pass, 2);
		}
		else if(g_Interfaces.GameConfigInstance->GetMemSig("SetUserCvar", &addr)) {
			sdktools__SetUserCvarCallWrapper = g_Interfaces.BinToolsInstance->CreateCall(addr, CallConv_ThisCall, NULL, pass, 2);
		}
		else {
			throw SDKToolsModSupportNotAvailableExceptionType("SetUserCvar");
		}
	}

#if SOURCE_ENGINE == SE_DARKMESSIAH
	if (NULL == sdktools__UpdateUserSettingsCallWrapper) {
		int offset;
		void *addr;
		
		if (g_Interfaces.GameConfigInstance->GetOffset("UpdateUserSettings", &offset)) {
			sdktools__UpdateUserSettingsCallWrapper = g_Interfaces.BinToolsInstance->CreateVCall(offset, 0, 0, NULL, NULL, 0);
		}
		else if(g_Interfaces.GameConfigInstance->GetMemSig("UpdateUserSettings", &addr)) {
			sdktools__UpdateUserSettingsCallWrapper = g_Interfaces.BinToolsInstance->CreateCall(addr, CallConv_ThisCall, NULL, NULL, 0);
		}
		else {
			throw SDKToolsModSupportNotAvailableExceptionType("UpdateUserSettings");
		}
	}
#else
	static int changedOffset = -1;

	if (changedOffset == -1) {	
		if (!g_Interfaces.GameConfigInstance->GetOffset("InfoChanged", &changedOffset)) {
			throw SDKToolsModSupportNotAvailableExceptionType("InfoChanged");
		}
	}
#endif

	SourceMod::IGamePlayer *gamePlayer = playerhelpers->GetGamePlayer(clientIndex);

	IClient *pClient = server->GetClient(clientIndex - 1);

	if(!gamePlayer->IsConnected() || !pClient) {
		throw ClientNotConnectedExceptionType(clientIndex);
	}

	unsigned char *CGameClient = (unsigned char *)pClient - 4;

	unsigned char vstk[sizeof(CBaseEntity *) + sizeof(const char *) + sizeof(const char *)];
	unsigned char *vptr = vstk;

	*(CBaseEntity **)vptr = (CBaseEntity*)CGameClient;
	vptr += sizeof(CBaseEntity *);
	*(const char **)vptr = key.c_str();
	vptr += sizeof(const char *);
	*(const char **)vptr = value.c_str();

	sdktools__SetUserCvarCallWrapper->Execute(vstk, NULL);

#if SOURCE_ENGINE == SE_DARKMESSIAH
	unsigned char vstk2[sizeof(void *)];
	unsigned char *vptr2 = vstk2;

	*(void **)vptr2 = CGameClient;
	sdktools__UpdateUserSettingsCallWrapper->Execute(vstk2, NULL);
#else
	uint8_t* changed = (uint8_t *)(CGameClient + changedOffset);
	*changed = 1;
#endif
}

DEFINE_CUSTOM_EXCEPTION_INIT(IServerNotFoundExceptionType, SDKTools)
DEFINE_CUSTOM_EXCEPTION_INIT(LightStyleOutOfRangeExceptionType, SDKTools)
DEFINE_CUSTOM_EXCEPTION_INIT(SDKToolsModSupportNotAvailableExceptionType, SDKTools)
DEFINE_CUSTOM_EXCEPTION_INIT(MapMustBeRunningExceptionType, SDKTools)
DEFINE_CUSTOM_EXCEPTION_INIT(InvalidStringTableExceptionType, SDKTools)
DEFINE_CUSTOM_EXCEPTION_INIT(InvalidStringTableStringIndexExceptionType, SDKTools)
DEFINE_CUSTOM_EXCEPTION_INIT(InvalidTempEntExceptionType, SDKTools)
DEFINE_CUSTOM_EXCEPTION_INIT(NoTempEntCallInProgressExceptionType, SDKTools)
DEFINE_CUSTOM_EXCEPTION_INIT(InvalidTempEntPropertyExceptionType, SDKTools)
DEFINE_CUSTOM_EXCEPTION_INIT(TempEntHookDoesNotExistExceptionType, SDKTools)
DEFINE_CUSTOM_EXCEPTION_INIT(EntityOutputClassNameHookDoesNotExistExceptionType, SDKTools)
DEFINE_CUSTOM_EXCEPTION_INIT(InvalidTeamExceptionType, SDKTools)

CDetour *sdktools__FireOutputDetour = NULL;

BOOST_PYTHON_MODULE(SDKTools) {
	py::enum_<ListenOverride>("ListenOverride")
		.value("Default", Listen_Default)
		.value("No", Listen_No)
		.value("Yes", Listen_Yes);
	
	py::class_<TraceResultsType>("TraceResults", py::no_init)
		.def_readonly("Fraction", &TraceResultsType::Fraction)
		.def_readonly("EndPosition", &TraceResultsType::EndPosition)
		.def_readonly("EntityIndex", &TraceResultsType::EntityIndex)
		.def_readonly("DidHit", &TraceResultsType::DidHit)
		.def_readonly("HitGroup", &TraceResultsType::HitGroup)
		.def_readonly("PlaneNormal", &TraceResultsType::PlaneNormal);

	py::class_<NetStatsType>("NetStats", py::no_init)
		.def_readonly("In", &NetStatsType::In)
		.def_readonly("Out", &NetStatsType::Out);

	py::class_<PointContentsType>("PointContents", py::no_init)
		.def_readonly("EntityIndex", &PointContentsType::EntityIndex)
		.def_readonly("ContentsMask", &PointContentsType::ContentsMask);
		
	py::def("InactivateClient", &sdktools__inactive_client, (py::arg("client_index")));
	py::def("ReconnectClient", &sdktools__reconnect_client, (py::arg("client_index")));
	py::def("SetClientViewEntity", &sdktools__set_client_view_entity, (py::arg("client_index"), py::arg("entity_index")));
	py::def("SetLightStyle", &sdktools__set_light_style, (py::arg("style"), py::arg("light_str")));
	py::def("GetClientEyePosition", &sdktools__get_client_eye_position, (py::arg("client_index")));
	py::def("AcceptEntityInput", &sdktools__accept_entity_input, (py::arg("entity_index"), py::arg("action"), py::arg("activator") = -1, py::arg("caller") = -1, py::arg("output_id") = 0));
	py::def("SetVariantBool", &sdktools__set_variant_bool, (py::arg("value")));
	py::def("SetVariantString", &sdktools__set_variant_string, (py::arg("value")));
	py::def("SetVariantInt", &sdktools__set_variant_int, (py::arg("value")));
	py::def("SetVariantFloat", &sdktools__set_variant_float, (py::arg("value")));
	py::def("SetVariantVector", &sdktools__set_variant_vector, (py::arg("value")));
	py::def("SetVariantPositionVector", &sdktools__set_variant_position_vector, (py::arg("value")));
	py::def("SetVariantColor", &sdktools__set_variant_color, (py::arg("value")));
	py::def("SetVariantEntity", &sdktools__set_variant_entity, (py::arg("value")));
	py::def("GiveClientItem", &sdktools__give_client_item, (py::arg("client_index"), py::arg("item"), py::arg("sub_type") = 0));
	py::def("RemoveClientItem", &sdktools__remove_client_item, (py::arg("client_index"), py::arg("item_entity_index")));
	py::def("IgniteEntity", &sdktools__ignite_entity, (py::arg("entity_index"), py::arg("time"), py::arg("npc") = false, py::arg("size") = 0.0f, py::arg("level") = false));
	py::def("ExtinguishEntity", &sdktools__extinguish_entity, (py::arg("entity_index")));
	py::def("ForceClientSuicide", &sdktools__force_client_suicide, (py::arg("client_index"), py::arg("explode") = false, py::arg("force") = false));
	py::def("TeleportEntity", &sdktools__teleport_entity, (py::arg("entity_index"), py::arg("origin") = BOOST_PY_NONE, py::arg("angles") = BOOST_PY_NONE, py::arg("velocity") = BOOST_PY_NONE));
	py::def("GetClientWeaponSlot", &sdktools__get_client_weapon_slot, (py::arg("client_index"), py::arg("weapon_slot")));
	py::def("ActivateEntity", &sdktools__activate_entity, (py::arg("entity_index")));
	py::def("EquipClientWeapon", &sdktools__equip_client_weapon, (py::arg("client_index"), py::arg("weapon_entity_index")));
	py::def("CreateEntityByName", &sdktools__create_entity_by_name, (py::arg("name")));
	py::def("DispatchKeyValue", &sdktools__dispatch_key_value, (py::arg("entity_index"), py::arg("key"), py::arg("value")));
	py::def("DispatchKeyValueFloat", &sdktools__dispatch_key_value_float, (py::arg("entity_index"), py::arg("key"), py::arg("value")));
	py::def("DispatchKeyValueVector", &sdktools__dispatch_key_value_vector, (py::arg("entity_index"), py::arg("key"), py::arg("value")));
	py::def("DispatchSpawn", &sdktools__dispatch_spawn, (py::arg("entity_index")));
	py::def("SetEntityModel", &sdktools__set_entity_model, (py::arg("entity_index"), py::arg("model")));
	py::def("GetClientDecalFile", &sdktools__get_client_decal_file, (py::arg("client_index")));
	py::def("FindStringTable", &sdktools__find_string_table, (py::arg("name")));
	py::def("GetNumStringTables", &sdktools__get_num_string_tables);
	py::def("GetStringTableNumStrings", &sdktools__get_string_table_num_strings, (py::arg("table_index")));
	py::def("GetStringTableMaxStrings", &sdktools__get_string_table_max_strings, (py::arg("table_index")));
	py::def("GetStringTableName", &sdktools__get_string_table_name, (py::arg("table_index")));
	py::def("FindStringIndex", &sdktools__find_string_index, (py::arg("table_index"), py::arg("needle")));
	py::def("ReadStringTable", &sdktools__read_string_table, (py::arg("table_index"), py::arg("string_index")));
	py::def("GetStringTableDataLength", &sdktools__get_string_table_data_length, (py::arg("table_index"), py::arg("string_index")));
	py::def("GetStringTableData", &sdktools__get_string_table_data, (py::arg("table_index"), py::arg("string_index")));
	py::def("SetStringTableData", &sdktools__set_string_table_data, (py::arg("table_index"), py::arg("string_index"), py::arg("value"), py::arg("length") = -1));
	py::def("AddToStringTable", &sdktools__add_to_string_table, (py::arg("table_index"), py::arg("str"), py::arg("userdata") = std::string(), py::arg("length") = -1));
	py::def("LockStringTables", &sdktools__lock_string_tables, (py::arg("lock")));
	py::def("GetPointContents", &sdktools__get_point_contents, (py::arg("position")));
	py::def("GetPointContentsEntity", &sdktools__get_point_contents_entity, (py::arg("entity_index"), py::arg("position")));
	py::def("GetMaxTracePoint", &sdktools__get_max_trace_point, (py::arg("position"), py::arg("angle")));
	py::def("TraceRay", &sdktools__trace_ray, (py::arg("start_position"), py::arg("end_position"), py::arg("flags"), py::arg("filter") = BOOST_PY_NONE));
	py::def("TraceHull", &sdktools__trace_hull, (py::arg("start_position"), py::arg("end_position"), py::arg("mins"), py::arg("maxs"), py::arg("flags"), py::arg("filter") = BOOST_PY_NONE));
	py::def("GetClientEyeAngles", &sdktools__get_client_eye_angles, (py::arg("client_index")));
	py::def("IsPointOutsideWorld", &sdktools__is_point_outside_world, (py::arg("position")));
	py::def("SetClientListeningFlags", &sdktools__set_client_listening_flags, (py::arg("client_index"), py::arg("flags")));
	py::def("GetClientListeningFlags", &sdktools__get_client_listening_flags, (py::arg("client_index")));
	py::def("SetListenOverride", &sdktools__set_listen_override, (py::arg("listener_client_index"), py::arg("sender_client_index"), py::arg("override")));
	py::def("GetListenOverride", &sdktools__get_listen_override, (py::arg("listener_client_index"), py::arg("sender_client_index")));
	py::def("IsClientMuted", &sdktools__is_client_muted, (py::arg("muter_client_index"), py::arg("mutee_client_index")));
	py::def("TEIsValid", &sdktools__te_is_valid, (py::arg("name")));
	py::def("TEStart", &sdktools__te_start, (py::arg("name")));
	py::def("TEIsValidProp", &sdktools__te_is_valid_prop, (py::arg("prop")));
	py::def("TEWriteNum", &sdktools__te_write_num, (py::arg("prop"), py::arg("num")));
	py::def("TEReadNum", &sdktools__te_read_num, (py::arg("prop")));
	py::def("TEWriteFloat", &sdktools__te_write_float, (py::arg("prop"), py::arg("float")));
	py::def("TEReadFloat", &sdktools__te_read_float, (py::arg("prop")));
	py::def("TEWriteVector", &sdktools__te_write_vector, (py::arg("prop"), py::arg("vec")));
	py::def("TEReadVector", &sdktools__te_read_vector, (py::arg("prop")));
	py::def("TESend", &sdktools__te_send, (py::arg("clients"), py::arg("delay") = 0.0f));
	py::def("GetGameRulesInt", &sdktools__get_game_rules_int, (py::arg("offset")));
	py::def("GetGameRulesUnsignedInt", &sdktools__get_game_rules_unsigned_int, (py::arg("offset")));
	py::def("GetGameRulesShort", &sdktools__get_game_rules_short, (py::arg("offset")));
	py::def("GetGameRulesUnsignedShort", &sdktools__get_game_rules_unsigned_short, (py::arg("offset")));
	py::def("GetGameRulesChar", &sdktools__get_game_rules_char, (py::arg("offset")));
	py::def("GetGameRulesUnsignedChar", &sdktools__get_game_rules_unsigned_char, (py::arg("offset")));
	py::def("GetGameRulesFloat", &sdktools__get_game_rules_float, (py::arg("offset")));
	py::def("GetGameRulesEntity", &sdktools__get_game_rules_entity, (py::arg("offset")));
	py::def("GetGameRulesVector", &sdktools__get_game_rules_vector, (py::arg("offset")));
	py::def("GetGameRulesString", &sdktools__get_game_rules_string, (py::arg("offset")));
	py::def("SetGameRulesInt", &sdktools__set_game_rules_int, (py::arg("offset"), py::arg("new_value"), py::arg("change_state") = false));
	py::def("SetGameRulesUnsignedInt", &sdktools__set_game_rules_unsigned_int, (py::arg("offset"), py::arg("new_value"), py::arg("change_state") = false));
	py::def("SetGameRulesShort", &sdktools__set_game_rules_short, (py::arg("offset"), py::arg("new_value"), py::arg("change_state") = false));
	py::def("SetGameRulesUnsignedShort", &sdktools__set_game_rules_unsigned_short, (py::arg("offset"), py::arg("new_value"), py::arg("change_state") = false));
	py::def("SetGameRulesChar", &sdktools__set_game_rules_char, (py::arg("offset"), py::arg("new_value"), py::arg("change_state") = false));
	py::def("SetGameRulesUnsignedChar", &sdktools__set_game_rules_unsigned_char, (py::arg("offset"), py::arg("new_value"), py::arg("change_state") = false));
	py::def("SetGameRulesFloat", &sdktools__set_game_rules_float, (py::arg("offset"), py::arg("new_value"), py::arg("change_state") = false));
	py::def("SetGameRulesEntity", &sdktools__set_game_rules_entity, (py::arg("offset"), py::arg("entity_index"), py::arg("change_state") = false));
	py::def("SetGameRulesVector", &sdktools__set_game_rules_vector, (py::arg("offset"), py::arg("new_value"), py::arg("change_state") = false));
	py::def("SetGameRulesString", &sdktools__set_game_rules_string, (py::arg("offset"), py::arg("new_value"), py::arg("max_length"), py::arg("change_state") = false));
	py::def("AddTempEntHook", &sdktools__add_temp_ent_hook, (py::arg("effect_name"), py::arg("callback")));
	py::def("RemoveTempEntHook", &sdktools__remove_temp_ent_hook, (py::arg("effect_name"), py::arg("callback")));
	py::def("HookEntityOutput", &sdktools__hook_entity_output, (py::arg("class_name"), py::arg("output"), py::arg("callback")));
	py::def("UnhookEntityOutput", &sdktools__unhook_entity_output, (py::arg("class_name"), py::arg("output"), py::arg("callback")));
	py::def("PrefetchSound", &sdktools__prefetch_sound, (py::arg("name")));
	py::def("EmitAmbientSound", &sdktools__emit_ambient_sound, (py::arg("name"), py::arg("position"), py::arg("entity_index") = 0, py::arg("level") = 75, py::arg("flags") = 0, py::arg("volume") = 1.0f, py::arg("pitch") = 100, py::arg("delay") = 0.0f));
	py::def("FadeClientVolume", &sdktools__fade_client_volume, (py::arg("client_index"), py::arg("percent"), py::arg("out_time"), py::arg("hold_time"), py::arg("in_time")));
	py::def("StopSound", &sdktools__stop_sound, (py::arg("entity_index"), py::arg("channel"), py::arg("name")));
	py::def("EmitSound", &sdktools__emit_sound, (py::arg("clients"), py::arg("sample"), py::arg("entity_index") = 0, py::arg("channel") = 0, py::arg("level") = 75, py::arg("flags") = 0, py::arg("volume") = 1.0f, py::arg("pitch") = 100, py::arg("speakerEntityIndex") = -1, py::arg("origin") = BOOST_PY_NONE, py::arg("direction") = BOOST_PY_NONE, py::arg("update_position") = true, py::arg("sound_time") = 0.0f, py::arg("additional_origins") = py::list()));
	py::def("EmitSentence", &sdktools__emit_sentence, (py::arg("clients"), py::arg("sentence"), py::arg("entity_index"), py::arg("channel") = 0, py::arg("level") = 75, py::arg("flags") = 0, py::arg("volume") = 1.0f, py::arg("pitch") = 100, py::arg("speakerEntityIndex") = -1, py::arg("origin") = BOOST_PY_NONE, py::arg("direction") = BOOST_PY_NONE, py::arg("update_position") = true, py::arg("sound_time") = 0.0f, py::arg("additional_origins") = py::list()));
	py::def("GetDistGainFromSoundLevel", &sdktools__get_dist_gain_from_sound_level, (py::arg("level"), py::arg("distance")));
	py::def("FindEntitiesByClassName", &sdktools__find_entities_by_class_name, (py::arg("class_name")));
	py::def("GetTeamCount", &sdktools__get_team_count);
	py::def("GetTeamName", &sdktools__get_team_name, (py::arg("team_index")));
	py::def("GetTeamScore", &sdktools__get_team_score, (py::arg("team_index")));
	py::def("SetTeamScore", &sdktools__set_team_score, (py::arg("team_index"), py::arg("new_score")));
	py::def("GetTeamClientCount", &sdktools__get_team_client_count, (py::arg("team_index")));
	py::def("GetServerNetStats", &sdktools__get_server_net_stats);
	py::def("SetClientInfo", &sdktools__set_client_info, (py::arg("client_index"), py::arg("key"), py::arg("value")));

	DEFINE_CUSTOM_EXCEPTION(IServerNotFoundExceptionType, SDKTools,
		PyExc_Exception, "SDKTools.IServerNotFoundException",
		"IServerNotFoundException")

	DEFINE_CUSTOM_EXCEPTION(LightStyleOutOfRangeExceptionType, SDKTools,
		PyExc_Exception, "SDKTools.LightStyleOutOfRangeException",
		"LightStyleOutOfRangeException")

	DEFINE_CUSTOM_EXCEPTION(SDKToolsModSupportNotAvailableExceptionType, SDKTools,
		PyExc_Exception, "SDKTools.SDKToolsModSupportNotAvailableException",
		"SDKToolsModSupportNotAvailableException")

	DEFINE_CUSTOM_EXCEPTION(MapMustBeRunningExceptionType, SDKTools,
		PyExc_Exception, "SDKTools.MapMustBeRunningException",
		"MapMustBeRunningException")

	DEFINE_CUSTOM_EXCEPTION(InvalidStringTableExceptionType, SDKTools,
		PyExc_Exception, "SDKTools.InvalidStringTableException",
		"InvalidStringTableException")

	DEFINE_CUSTOM_EXCEPTION(InvalidStringTableStringIndexExceptionType, SDKTools,
		PyExc_Exception, "SDKTools.InvalidStringTableStringIndexException",
		"InvalidStringTableStringIndexException")

	DEFINE_CUSTOM_EXCEPTION(InvalidTempEntExceptionType, SDKTools,
		PyExc_Exception, "SDKTools.InvalidTempEntException",
		"InvalidTempEntException")

	DEFINE_CUSTOM_EXCEPTION(NoTempEntCallInProgressExceptionType, SDKTools,
		PyExc_Exception, "SDKTools.NoTempEntCallInProgressException",
		"NoTempEntCallInProgressException")

	DEFINE_CUSTOM_EXCEPTION(InvalidTempEntPropertyExceptionType, SDKTools,
		PyExc_Exception, "SDKTools.InvalidTempEntPropertyException",
		"InvalidTempEntPropertyException")

	DEFINE_CUSTOM_EXCEPTION(TempEntHookDoesNotExistExceptionType, SDKTools,
		PyExc_Exception, "SDKTools.TempEntHookDoesNotExistException",
		"TempEntHookDoesNotExistException")

	DEFINE_CUSTOM_EXCEPTION(EntityOutputClassNameHookDoesNotExistExceptionType, SDKTools,
		PyExc_Exception, "SDKTools.EntityOutputClassNameHookDoesNotExistException",
		"EntityOutputClassNameHookDoesNotExistException")

	DEFINE_CUSTOM_EXCEPTION(InvalidTeamExceptionType, SDKTools,
		PyExc_Exception, "SDKTools.InvalidTeamException",
		"InvalidTeamException")

	memset(sdktools__VoiceMap, 0, sizeof(sdktools__VoiceMap));
	memset(sdktools__ClientMutes, 0, sizeof(sdktools__ClientMutes));

	sdktools__GameRulesProxyClassName = std::string(g_Interfaces.GameConfigInstance->GetKeyValue("GameRulesProxy"));

	playerhelpers->AddClientListener(&sdktools__ClientListener);

	SH_ADD_HOOK(IVEngineServer, PlaybackTempEntity, engine, SH_STATIC(&sdktools__OnPlaybackTempEntity), false);

	sdktools__FireOutputDetour = DETOUR_CREATE_MEMBER(FireOutput, "FireOutput");
	sdktools__FireOutputDetour->EnableDetour();
}

void destroySDKTools() {
	playerhelpers->RemoveClientListener(&sdktools__ClientListener);

	SH_REMOVE_HOOK(IVEngineServer, PlaybackTempEntity, engine, SH_STATIC(&sdktools__OnPlaybackTempEntity), false);

	sdktools__FireOutputDetour->Destroy();
}

bool RemoveFirstTempEntHookByThreadState(PyThreadState *threadState) {
	for(std::list<TempEntHook>::iterator it = sdktools__TEHooks.begin();
		it != sdktools__TEHooks.end(); it++) {
		TempEntHook hookInfo = *it;

		if(hookInfo.ThreadState != threadState) {
			continue;
		}

		sdktools__TEHooks.erase(it);
		return true;
	}

	return false;
}

void RemoveAllTempEntHooksByThreadState(PyThreadState *threadState) {
	bool keepSearching = true;

	while(keepSearching) {
		keepSearching = RemoveFirstTempEntHookByThreadState(threadState);
	}
}

bool RemoveFirstEntityOutputClassNameHookByThreadState(PyThreadState *threadState) {
	for(std::list<EntityOutputClassNameHook>::iterator it = sdktools__EntityOutputClassNameHooks.begin();
		it != sdktools__EntityOutputClassNameHooks.end(); it++) {
		EntityOutputClassNameHook hookInfo = *it;

		if(hookInfo.ThreadState != threadState) {
			continue;
		}

		sdktools__EntityOutputClassNameHooks.erase(it);
		return true;
	}

	return false;
}

void RemoveAllEntityOutputClassNameHooksByThreadState(PyThreadState *threadState) {
	bool keepSearching = true;

	while(keepSearching) {
		keepSearching = RemoveFirstEntityOutputClassNameHookByThreadState(threadState);
	}
}

void unloadThreadStateSDKTools(PyThreadState *threadState) {
	RemoveAllTempEntHooksByThreadState(threadState);
	RemoveAllEntityOutputClassNameHooksByThreadState(threadState);
}