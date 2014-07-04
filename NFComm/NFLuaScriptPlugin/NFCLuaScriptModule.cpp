// -------------------------------------------------------------------------
//    @FileName      :    NFCLuaScriptModule.cpp
//    @Author           :    LvSheng.Huang
//    @Date             :    2013-01-02
//    @Module           :    NFCLuaScriptModule
//    @Desc             :
// -------------------------------------------------------------------------

#include "NFCLuaScriptModule.h"
#include "NFLuaScriptPlugin.h"
#include <assert.h>

bool NFCLuaScriptModule::Init()
{
    return true;
}

bool NFCLuaScriptModule::AfterInit()
{
    m_pEventProcessModule = dynamic_cast<NFIEventProcessModule*>(pPluginManager->FindModule("NFCEventProcessModule"));
    m_pKernelModule = dynamic_cast<NFIKernelModule*>(pPluginManager->FindModule("NFCKernelModule"));
    m_pLogicClassModule = dynamic_cast<NFILogicClassModule*>(pPluginManager->FindModule("NFCLogicClassModule"));
    m_pElementInfoModule = dynamic_cast<NFIElementInfoModule*>(pPluginManager->FindModule("NFCElementInfoModule"));

    assert(NULL != m_pEventProcessModule);
    assert(NULL != m_pKernelModule);
    assert(NULL != m_pLogicClassModule);
    assert(NULL != m_pElementInfoModule);

    m_pScriptKernelModule = new NFCScriptKernelModule(m_pKernelModule, this, m_pElementInfoModule, m_pEventProcessModule);

    bool ret = InstallLua("script_init.lua");
    assert( ret );

    NFILogicClass* pClass = m_pLogicClassModule->First();
    while (pClass)
    {
        NFIComponent* pComponent = pClass->GetComponentManager()->First();
        while (pComponent)
        {
            if (!CheckCompomentStatus(pComponent->ComponentName()))
            {
                assert(0);
            }

            pComponent = pClass->GetComponentManager()->Next();
        }

        pClass = m_pLogicClassModule->Next();
    } 

    //add all callback
    m_pKernelModule->ResgisterCommonPropertyEvent(this, &NFCLuaScriptModule::OnPropertyCommEvent);
    m_pKernelModule->ResgisterCommonRecordEvent(this, &NFCLuaScriptModule::OnRecordCommonEvent);
    m_pKernelModule->ResgisterCommonClassEvent(this, &NFCLuaScriptModule::OnClassCommonEvent);    

    return true;
}

bool NFCLuaScriptModule::Shut()
{

    return true;
}

bool NFCLuaScriptModule::Execute(const float fLasFrametime, const float fStartedTime)
{
    return true;
}

bool NFCLuaScriptModule::BeforeShut()
{
    delete m_pScriptKernelModule;
    m_pScriptKernelModule = NULL;

    return true;
}

int NFCLuaScriptModule::OnPropertyCommEvent( const NFIDENTID& self, const std::string& strPropertyName, const NFIValueList& oldVar, const NFIValueList& newVar, const NFIValueList& arg )
{
    NFCSriptData* pScriptData = m_pScriptKernelModule->GetElement(self);
    if (pScriptData)
    {
        NFList<NFCScriptName>* pList = pScriptData->mxPropertyCallBackData.GetElement(strPropertyName);
        if (pList)
        {
            NFCScriptName xScriptName;
            bool bRet = pList->First(xScriptName);
            while (bRet)
            {
                DoScriptPropertyCallBack(self, strPropertyName, xScriptName.strComponentName, xScriptName.strFunctionName, NFCScriptVarList(oldVar), NFCScriptVarList(newVar), NFCScriptVarList(arg));

                bRet = pList->Next(xScriptName);
            }
        }
    }

    return 0;
}

int NFCLuaScriptModule::OnRecordCommonEvent( const NFIDENTID& self, const std::string& strRecordName, const int nOpType, const int nRow, const int nCol, const NFIValueList& oldVar, const NFIValueList& newVar, const NFIValueList& arg )
{
    NFCSriptData* pScriptData = m_pScriptKernelModule->GetElement(self);
    if (pScriptData)
    {
        NFList<NFCScriptName>* pList = pScriptData->mxRecordCallBackData.GetElement(strRecordName);
        if (pList)
        {
            NFCScriptName xScriptName;
            bool bRet = pList->First(xScriptName);
            while (bRet)
            {
                DoScriptRecordCallBack(self, strRecordName, xScriptName.strComponentName, xScriptName.strFunctionName, nOpType, nRow, nCol, NFCScriptVarList(oldVar), NFCScriptVarList(newVar), NFCScriptVarList(arg));

                bRet = pList->Next(xScriptName);
            }
        }
    }

    return 0;
}

int NFCLuaScriptModule::OnClassCommonEvent( const NFIDENTID& self, const std::string& strClassName, const CLASS_OBJECT_EVENT eClassEvent, const NFIValueList& var )
{
    std::string strSerializationName;

    switch (eClassEvent)
    {
    case COE_CREATE_NODATA:
        strSerializationName = "Init";
        break;

    case COE_CREATE_HASDATA:
        strSerializationName = "AfterInit";
        break;

    case COE_BEFOREDESTROY:
        strSerializationName = "BeforeShut";
        break;

    case COE_DESTROY:
        strSerializationName = "Shut";
        break;

    default:
        break;
    }

    if (!strSerializationName.empty())
    {
        NFIComponentManager* pComponentManager = m_pLogicClassModule->GetClassComponentManager(strClassName);
        if (pComponentManager)
        {
            NFIComponent* pComponent = pComponentManager->First();
            while (pComponent && pComponent->Enable())
            {
                DoScript(self, pComponent->ComponentName(), strSerializationName, NFCScriptVarList(var));

                pComponent = pComponentManager->Next();
            }
        }
    }

    return 0;
}

int NFCLuaScriptModule::DoHeartBeatCommonCB( const NFIDENTID& self, const std::string& strHeartBeat, const float fTime, const int nCount, const NFIValueList& var )
{
    NFCSriptData* pScriptData = m_pScriptKernelModule->GetElement(self);
    if (pScriptData)
    {
        NFList<NFCScriptName>* pList = pScriptData->mxHeartBeatData.GetElement(strHeartBeat);
        if (pList)
        {
            NFCScriptName xScriptName;
            bool bRet = pList->First(xScriptName);
            while (bRet)
            {
                DoHeartBeatScript(self, strHeartBeat, fTime, nCount, xScriptName.strComponentName, xScriptName.strFunctionName, NFCScriptVarList(var));

                bRet = pList->Next(xScriptName);
            }
        }
    }

    return 0;
}

int NFCLuaScriptModule::DoEventCommonCB( const NFIDENTID& self, const int nEventID, const NFIValueList& var )
{
    NFCSriptData* pScriptData = m_pScriptKernelModule->GetElement(self);
    if (pScriptData)
    {
        NFList<NFCScriptName>* pList = pScriptData->mxEventData.GetElement(nEventID);
        if (pList)
        {
            NFCScriptName xScriptName;
            bool bRet = pList->First(xScriptName);
            while (bRet)
            {
                //(const NFIDENTID& self, const int nEventID, const std::string& strComponentName, const std::string& strFunction, const NFCScriptVarList& arg)
                DoEventScript(self, nEventID, xScriptName.strComponentName, xScriptName.strFunctionName, NFCScriptVarList(var));

                bRet = pList->Next(xScriptName);
            }
        }
    }

    return 0;
}

bool NFCLuaScriptModule::InstallLua(const std::string& strComponentName)
{
    using namespace luabind;

    mpLuaState = lua_open(); 
    luaL_openlibs(mpLuaState);
    luabind::open(mpLuaState);

    if ( !Regisger() )
    {
        lua_close(mpLuaState);
        mpLuaState = NULL;
        return false;
    }    

    if(luaL_loadfile(mpLuaState, strComponentName.c_str()) != 0)
    {
        lua_close(mpLuaState);
        mpLuaState = NULL;
        return false;
    }

    lua_pcall(mpLuaState, 0, 0, 0);

    return true;
}

bool NFCLuaScriptModule::CheckCompomentStatus(const std::string& strComponentName, const std::string& strFuncName)
{
    if( !CheckCompomentStatus(strComponentName))
    {
        return false;
    }

    //�Ƿ��к���
    luabind::object objCompoment = luabind::globals(mpLuaState)[strComponentName.c_str()];
    if (luabind::type(objCompoment) != LUA_TTABLE)
    {
        return false;
    }

    luabind::object objFun = objCompoment[strFuncName];
    if (luabind::type(objFun) != LUA_TFUNCTION)
    {
        return false;
    }

    return true;
}

bool NFCLuaScriptModule::CheckCompomentStatus( const std::string& strComponentName )
{
    if (!mmCompomentStatus.GetElement(strComponentName))
    { 
        int nRet = luabind::call_function<int>(mpLuaState, "load_script_file", strComponentName.c_str());
        if (nRet <= 0)
        {
            return false;
        }

        mmCompomentStatus.AddElement(strComponentName, new int(1));
    }

    return true;
}

int NFCLuaScriptModule::DoScript( const NFIDENTID& self, const std::string& strComponentName, const std::string& strFunction, const NFCScriptVarList& arg )
{
    if (!CheckCompomentStatus(strComponentName, strFunction))
    {
        return 0;
    }

    luabind::object objCompoment = luabind::globals(mpLuaState)[strComponentName.c_str()];
    luabind::call_member<int>(objCompoment, strFunction.c_str(), NFScriptInt64((int)m_pScriptKernelModule), NFScriptInt64(self), arg);

    return 1;
}

int NFCLuaScriptModule::DoEventScript( const NFIDENTID& self, const int nEventID, const std::string& strComponentName, const std::string& strFunction, const NFCScriptVarList& arg )
{
    if (!CheckCompomentStatus(strComponentName, strFunction))
    {
        return 0;
    }

    luabind::object objCompoment = luabind::globals(mpLuaState)[strComponentName.c_str()];
    luabind::call_member<int>(objCompoment, strFunction.c_str(), NFScriptInt64((int)m_pScriptKernelModule), NFScriptInt64(self), nEventID, arg);

    return 1;
}

int NFCLuaScriptModule::DoHeartBeatScript( const NFIDENTID& self, const std::string& strHeartBeat, const float fTime, const int nCount, std::string& strComponentName, const std::string& strFunction, const NFCScriptVarList& arg )
{
    if (!CheckCompomentStatus(strComponentName, strFunction))
    {
        return 0;
    }

    luabind::object objCompoment = luabind::globals(mpLuaState)[strComponentName.c_str()];
    luabind::call_member<int>(objCompoment, strFunction.c_str(), NFScriptInt64((int)m_pScriptKernelModule), NFScriptInt64(self), strHeartBeat.c_str(), fTime, nCount, arg);

    return 1;
}

int NFCLuaScriptModule::DoScriptPropertyCallBack( const NFIDENTID& self, const std::string& strPropertyName, const std::string& strComponentName, const std::string& strFunction, const NFCScriptVarList& oldVar, const NFCScriptVarList& newVar, const NFCScriptVarList& arg )
{
    if (!CheckCompomentStatus(strComponentName, strFunction))
    {
        return 0;
    }

    luabind::object objCompoment = luabind::globals(mpLuaState)[strComponentName.c_str()];
    luabind::call_member<int>(objCompoment, strFunction.c_str(), NFScriptInt64((int)m_pScriptKernelModule), NFScriptInt64(self), strPropertyName.c_str(), oldVar, newVar, arg);

    return 1;
}

int NFCLuaScriptModule::DoScriptRecordCallBack( const NFIDENTID& self, const std::string& strRecordName, const std::string& strComponentName, const std::string& strFunction, const int nOpType, const int nRow, const int nCol, const NFCScriptVarList& oldVar, const NFCScriptVarList& newVar, const NFCScriptVarList& arg )
{
    if (!CheckCompomentStatus(strComponentName, strFunction))
    {
        return 0;
    }

    luabind::object objCompoment = luabind::globals(mpLuaState)[strComponentName.c_str()];
    luabind::call_member<int>(objCompoment, strFunction.c_str(), NFScriptInt64((int)m_pScriptKernelModule), NFScriptInt64(self), strRecordName.c_str(), nOpType, nRow, nCol, oldVar, newVar, arg);

    return 1;
}


bool NFCLuaScriptModule::Regisger()
{
    using namespace luabind;

    module(mpLuaState)
        [  
            def ("NFVarList_New", &NFVarList_New),
            def ("NFVarList_Del", &NFVarList_Del),
            def ("NFVarList_Clear", &NFVarList_Clear),

            def ("HasEventCallBack", &KernelModule_HasEventCallBack),
            def ("RemoveEvent", &KernelModule_RemoveEvent),
            def ("RemoveEventCallBack", &KernelModule_RemoveEventCallBack),
            def ("DoEvent", &KernelModule_DoEvent),
            def ("ExistElement", &KernelModule_ExistElement),
            def ("QueryPropertyInt", (int(*)(const NFScriptInt64&, const std::string&, const std::string& ))&KernelModule_QueryPropertyInt),
            def ("QueryPropertyFloat", (float (*)(const NFScriptInt64& ,const std::string& , const std::string& ))&KernelModule_QueryPropertyFloat),
            def ("QueryPropertyString", ( const std::string& (*)(const NFScriptInt64&, const std::string&, const std::string&))&KernelModule_QueryPropertyString),
            def ("AddPropertyCallBack", &KernelModule_AddPropertyCallBack),
            def ("AddRecordCallBack", &KernelModule_AddRecordCallBack),
            def ("AddEventCallBack", &KernelModule_AddEventCallBack),
            def ("AddHeartBeat", &KernelModule_AddHeartBeat),
            def ("FindHeartBeat", &KernelModule_FindHeartBeat),
            def ("RemoveHeartBeat", &KernelModule_RemoveHeartBeat),
            def ("SetComponentEnable", &KernelModule_SetComponentEnable),
            def ("QueryComponentEnable", &KernelModule_QueryComponentEnable),
            def ("CreateContainer", &KernelModule_CreateContainer),
            def ("ExistContainer", &KernelModule_ExistContainer),
            def ("CreateObject", &KernelModule_CreateObject),
            def ("DestroyObject", &KernelModule_DestroyObject),
            def ("FindProperty", &KernelModule_FindProperty),
            def ("SetPropertyInt", &KernelModule_SetPropertyInt),
            def ("SetPropertyFloat", &KernelModule_SetPropertyFloat),
            def ("SetPropertyString", &KernelModule_SetPropertyString),
            def ("SetPropertyObject", &KernelModule_SetPropertyObject),        

            def ("QueryPropertyInt", (int (*)(const NFScriptInt64&, const NFScriptInt64&, const std::string&))&KernelModule_QueryPropertyInt),
            def ("QueryPropertyFloat", (float (*)(const NFScriptInt64&, const NFScriptInt64&,const std::string&))&KernelModule_QueryPropertyFloat),
            def ("QueryPropertyString", (const std::string& (*)(const NFScriptInt64&, const NFScriptInt64&,const std::string&))&KernelModule_QueryPropertyString),
            def ("QueryPropertyObject", &KernelModule_QueryPropertyObject),
            def ("SetRecordInt", &KernelModule_SetRecordInt),
            def ("SetRecordFloat", &KernelModule_SetRecordFloat),
            def ("SetRecordString", &KernelModule_SetRecordString),
            def ("SetRecordObject", &KernelModule_SetRecordObject),
            def ("QueryRecordInt", &KernelModule_QueryRecordInt),
            def ("QueryRecordFloat", &KernelModule_QueryRecordFloat),
            def ("QueryRecordString", &KernelModule_QueryRecordString),
            def ("QueryRecordObject", &KernelModule_QueryRecordObject),
            def ("AddProperty", &KernelModule_AddProperty),
            //        def ("KernelModule_AddRecord", &KernelModule_AddRecord),

            class_ <NFCScriptVarList> ("NFCVarList")
            .def(luabind::constructor<>())
            .def ("IsEmpty", &NFCScriptVarList::IsEmpty)
            .def ("GetCount", &NFCScriptVarList::GetCount)
            .def ("Type", &NFCScriptVarList::Type)
            .def ("AddInt", &NFCScriptVarList::AddInt)
            .def ("AddFloat", &NFCScriptVarList::AddFloat)
            .def ("AddString", &NFCScriptVarList::AddString)
            .def ("AddObject", &NFCScriptVarList::AddObject)
            .def ("SetInt", &NFCScriptVarList::SetInt)
            .def ("SetFloat", &NFCScriptVarList::SetFloat)
            .def ("SetString", &NFCScriptVarList::SetString)
            .def ("SetObject", &NFCScriptVarList::SetObject)
            .def ("IntVal", &NFCScriptVarList::IntVal)
            .def ("FloatVal", &NFCScriptVarList::FloatVal)
            .def("StringVal", &NFCScriptVarList::StringVal)
            .def("ObjectVal", &NFCScriptVarList::ObjectVal),

            class_ <NFScriptInt64> ("NFINT64")
            .def(luabind::constructor<>())
            .def ("GetA", &NFScriptInt64::GetA)    
            .def ("GetB", &NFScriptInt64::GetB)    
            .def ("SetA", &NFScriptInt64::SetA)    
            .def ("SetB", &NFScriptInt64::SetB)    
        ]; 

    return true;
}