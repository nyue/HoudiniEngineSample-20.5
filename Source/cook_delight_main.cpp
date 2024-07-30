#include "HoudiniApi.h"
#include "HoudiniEngineGeometry.h"
#include "HoudiniEngineManager.h"
#include "HoudiniEnginePlatform.h"
#include "HoudiniEngineUtility.h"

#include <iostream>
#include <string>

int
main(int argc, char ** argv)
{
    std::cout << "===================================" << std::endl;
    std::cout << " Houdini Engine Sample Application " << std::endl;
    std::cout << "===================================\n" << std::endl;

    // Dynamically load the libHAPIL and load the HAPI
    // functions exported from the dll
    void* libHAPIL = HoudiniEnginePlatform::LoadLibHAPIL();
    std::cout << "After HoudiniEnginePlatform::LoadLibHAPIL()\n" << std::endl;
	
    if (libHAPIL != nullptr)
        HoudiniApi::InitializeHAPI(libHAPIL);

    if (!HoudiniApi::IsHAPIInitialized())
    {
        std::cerr << "Failed to load and initialize the "
                     "Houdini Engine API from libHAPIL." << std::endl;
        return 1;
    }  

    int session_type = 1;

    bool use_cooking_thread = true; // Enables asynchronous cooking of nodes.
    std::string named_pipe = DEFAULT_NAMED_PIPE;
    int tcp_port = DEFAULT_TCP_PORT;
    std::string shared_mem_name;

    if (session_type == HoudiniEngineManager::SessionType::ExistingNamedPipe)
    {
        std::cout << "Please specify the pipe name:" << std::endl;
        std::cout << ">> ";
        std::cin >> named_pipe;
    }
    else if (session_type == HoudiniEngineManager::SessionType::ExistingTCPSocket)
    {
        std::cout << "Please specify the TCP port:" << std::endl;
        std::cout << ">> ";
        std::cin >> tcp_port;
    }
    else if (session_type == HoudiniEngineManager::SessionType::ExistingSharedMemory)
    {
        std::cout << "Please specify the shared memory name:" << std::endl;
        std::cout << ">> ";
        std::cin >> shared_mem_name;
    }

    HoudiniEngineManager* he_manager = new HoudiniEngineManager();
    if (!he_manager)
    {
        std::cerr << "Failed to create the Houdini Engine Manager." << std::endl;
        return 1;
    }

    if (!he_manager->startSession(
        (HoudiniEngineManager::SessionType)session_type, named_pipe, tcp_port, shared_mem_name))
    {
        std::cerr << "Failed to create a Houdini Engine session." << std::endl;
        return 1;
    }

    if (!he_manager->initializeHAPI(use_cooking_thread))
    {
        std::cerr << "Failed to initialize HAPI." << std::endl;
        return 1;
    }

    std::cout << "\nLoading the hexagona sample HDA: " << std::endl;
    
    std::string otl_path = HDA_INSTALL_PATH + std::string("/hexagona_lite.hda");
    HAPI_AssetLibraryId asset_id = -1;
    std::string asset_name;
    if (!he_manager->loadAsset(otl_path.c_str(), asset_id, asset_name))
    {
        std::cerr << "Failed to load the default HDA (" << otl_path << ")." << std::endl;
        return 1;
    }

    std::string user_cmd;

    bool hexagona_cook = false;
    HAPI_NodeId hexagona_node_id = 0;
    HAPI_PartId hexagona_part_id = 0;

    bool mesh_data_generated = false;
    HAPI_NodeId input_mesh_node_id = 0;

	hexagona_cook = he_manager->createAndCookNode(asset_name.c_str(), &hexagona_node_id);
	he_manager->exportDelight(hexagona_node_id, hexagona_part_id);
	
    he_manager->stopSession();
    delete he_manager;

    HoudiniApi::FinalizeHAPI();
    HoudiniEnginePlatform::FreeLibHAPIL(libHAPIL);

    return 0;
}
