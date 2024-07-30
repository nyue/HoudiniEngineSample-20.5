#include "HoudiniApi.h"
#include "HoudiniEngineGeometry.h"
#include "HoudiniEngineManager.h"
#include "HoudiniEnginePlatform.h"
#include "HoudiniEngineUtility.h"

#include <iostream>
#include <string>

#include "nsi_procedural.h"

#include "nsi_dynamic.hpp"
#include "nsi_util.h"

#include <math.h>

int
_main(int argc, char ** argv)
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

// ============================================================================
// Extends NSIProcedural_t to store private data
struct GearProcedural : public NSIProcedural_t
{
	explicit GearProcedural(const char* nsi_library_path)
		:	api(nsi_library_path)
	{
	}

	/*
		This loads symbols from the calling NSI library directly.
		It avoids having to link the procedural against an NSI library and
		letting the operating system locate it later.
		If linking the procedural against NSI is not a problem, then this can
		be omitted and the regular NSI API can be used directly.
	*/
	NSI::DynamicAPI api;
};


static NSI_PROCEDURAL_UNLOAD(gear_unload)
{
	/*
		The "proc" parameter is actually a pointer to the GearProcedural object
		allocated in NSIProceduralLoad.
	*/
	GearProcedural* gproc = (GearProcedural*)proc;
	delete gproc;
}


static NSI_PROCEDURAL_EXECUTE(gear_execute)
{
	/*
		The "proc" parameter is actually a pointer to the GearProcedural object
		allocated in NSIProceduralLoad.
	*/
	GearProcedural* gproc = (GearProcedural*)proc;

	// Retrieve parameters using utility functions from nsi_util.h
	const char* parent_node =
		NSI::FindStringParameter("parentnode", nparams, params);
	const char* node =
		NSI::FindStringParameter("node", nparams, params);
	const int* nb_teeth =
		NSI::FindIntegerParameter("nb_teeth", nparams, params);
	const float* inner_radius =
		NSI::FindFloatParameter("inner_radius", nparams, params);
	const float* outer_radius =
		NSI::FindFloatParameter("outer_radius", nparams, params);
	const float* teeth_slope =
		NSI::FindFloatParameter("teeth_slope", nparams, params);

	// Validate parameters and report errors

	if(!parent_node)
	{
		/*
			This is normal when the procedural is executed through a procedural
			node instead of a call to NSIEvaluate.
		*/
		parent_node = NSI_SCENE_ROOT;
	}

	if(!node)
	{
		/*
			This is normal when the procedural is executed through a procedural
			node instead of a call to NSIEvaluate.
			In that case, since the procedural is evaluated inside its own NSI
			context, in isolation from the main scene, there is no need to
			specify the new node's handle from outside the procedural.
		*/
		node = "gear";
	}

	if(!nb_teeth || *nb_teeth < 6)
	{
		report(ctx, NSIErrError, "gear : invalid number of teeth");
		return;
	}

	if(!inner_radius || *inner_radius <= 0.0f)
	{
		report(ctx, NSIErrError, "gear : invalid inner radius");
		return;
	}

	if(!outer_radius || *outer_radius <= *inner_radius)
	{
		report(ctx, NSIErrError, "gear : invalid outer radius");
		return;
	}

	float slope = teeth_slope ? *teeth_slope : 0.75f;
	if(slope <= 0.0f || slope > 1.0f)
	{
		report(ctx, NSIErrError, "gear : invalid teeth slope");
		return;
	}

	// Build the positions vector	
	std::vector<float> P;
	float pitch = 2.0f * float(M_PI) / float(*nb_teeth);
	float inner_offset = pitch * (1.0f / (1.0f+slope)) / 2.0f;
	float outer_offset =
		pitch * (slope / (1.0f+slope)) / 2.0f * *inner_radius / *outer_radius;
	for(int v = 0; v < *nb_teeth; v++)
	{
		float tooth_axis = v*pitch;

		P.push_back(-sinf(tooth_axis-inner_offset) * *inner_radius);
		P.push_back(cosf(tooth_axis-inner_offset) * *inner_radius);
		P.push_back(0.0f);

		P.push_back(-sinf(tooth_axis-outer_offset) * *outer_radius);
		P.push_back(cosf(tooth_axis-outer_offset) * *outer_radius);
		P.push_back(0.0f);

		P.push_back(-sinf(tooth_axis+outer_offset) * *outer_radius);
		P.push_back(cosf(tooth_axis+outer_offset) * *outer_radius);
		P.push_back(0.0f);

		P.push_back(-sinf(tooth_axis+inner_offset) * *inner_radius);
		P.push_back(cosf(tooth_axis+inner_offset) * *inner_radius);
		P.push_back(0.0f);
	}

	/*
		Initialize a NSI::Context wrapper object from the DynamicAPI and the
		actual context handle (see comment in NSIProceduralLoad).
		If the procedural was to be linked directly against an NSI
		implementation, we could have omitted NSI::Context's constructor
		parameter.
	*/
	NSI::Context nsi(gproc->api);
	nsi.SetHandle(ctx);

	// Create the mesh and set its attributes
	nsi.Create(node, "mesh");
	nsi.SetAttribute(node,
		(
			NSI::IntegerArg("nvertices", P.size()/3),
			NSI::PointsArg("P", &P[0], P.size()/3)
		) );

	// Connect it to its parent transform
	nsi.Connect(node, "", parent_node, "objects");
}


// Main procedural entry point
NSI_PROCEDURAL_LOAD
{
	GearProcedural* proc = new GearProcedural(nsi_library_path);
	NSI_PROCEDURAL_INIT(*proc, gear_unload, gear_execute);

	return proc;
}
