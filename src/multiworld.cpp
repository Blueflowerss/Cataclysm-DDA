
#include <climits>
#include <cstdint>
#include <iosfwd>
#include <new>
#include <optional>
#include <string>
#include <vector>

#include <multiworld.h>
#include <map.h>
#include <avatar.h>
#include <game.h>
#include <weather.h>
#include <mapbuffer.h>
#include <overmapbuffer.h>
#include "json.h"
#include "json_loader.h"

multiworld::multiworld() = default;
multiworld::~multiworld() = default;
multiworld MULTIWORLD;

void multiworld::set_world_prefix( std::string prefix )
{
    world_prefix = std::move( prefix );
}
std::string multiworld::get_world_prefix()
{
    return world_prefix;
}
//doesn't actually create a world
bool multiworld::create_or_modify_world( const std::string &prefix )
{
    subworld_manifest[prefix] = subworld_settings();
    return true;
}
void multiworld::adjust_time( const std::string &prefix )
{

}

bool multiworld::travel_to_world( const std::string &prefix )
{
    map &here = get_map();
    avatar &player = get_avatar();
    //unload NPCs
    g->unload_npcs();
    //unload monsters
    for( monster &critter : g->all_monsters() ) {
        g->despawn_monster( critter );
    }
    if( player.in_vehicle ) {
        here.unboard_vehicle( player.pos_bub() );
    }
    //make sure we don't mess up savedata if for some reason maps can't be saved
    if( !g->save_maps() ) {
        return false;
    }
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
        here.clear_vehicle_list( z );
    }
    here.rebuild_vehicle_level_caches();
    /*inputting an empty string to the text input EOC fails
    so i'm using 'default' as empty/main world */
    if( prefix != "default" ) {
        set_world_prefix( prefix );
    } else {
        set_world_prefix( "" );
    }
    MAPBUFFER.clear();
    //FIXME hack to prevent crashes from temperature checks
    //this returns to false in 'on_turn()' so it should be fine?
    g->swapping_worlds = true;
    //in theory if we skipped the next two lines we'd have an exact copy of the overmap from the past world, only with differences noticeable in the local map.
    overmap_buffer.clear();
    //load/create new overmap
    overmap_buffer.get( point_abs_om{} );
    //loads submaps
    here.load( tripoint_abs_sm( here.get_abs_sub() ), false );
    here.access_cache( here.get_abs_sub().z() ).map_memory_cache_dec.reset();
    here.access_cache( here.get_abs_sub().z() ).map_memory_cache_ter.reset();
    here.invalidate_visibility_cache();
    g->load_npcs();
    here.spawn_monsters( true, true ); // Static monsters
    g->update_overmap_seen();
    // update weather now as it could be different on the new location
    get_weather().nextweather = calendar::turn;
    return true;
}
std::string multiworld::get_world_region_type( const std::string &world_prefix )
{
    const std::string region_type = subworld_manifest[world_prefix].region_type;
    if( region_settings_map.find( region_type ) != region_settings_map.end() ) {
        return region_type;
    }
    return "default";
}
std::string multiworld::get_current_world_region_type()
{
    const std::string region_type = subworld_manifest[MULTIWORLD.world_prefix].region_type;
    if( region_settings_map.find( region_type ) != region_settings_map.end() ) {
        return region_type;
    }
    return "default";
}
bool multiworld::load_subworld_manifest()
{
    read_from_file_optional_json(
        PATH_INFO::world_base_save_path_path() / "subworld_manifest.json",
    []( const JsonValue & jsin ) {
        JsonObject jo = jsin.get_object();
        for( auto it = jo.begin(); it != jo.end(); ++it ) {
            JsonMember memb = *it;
            JsonObject world_settings = jo.get_object( memb.name() );
            MULTIWORLD.create_or_modify_world( memb.name() );
            subworld_settings &loadedWorld = MULTIWORLD.subworld_manifest[memb.name()];
            loadedWorld.region_type = world_settings.get_string( "region_type" );
            loadedWorld.is_temporary = world_settings.get_bool( "is_temporary" );
            loadedWorld.parallel_world = world_settings.get_bool( "parallel_world" );
        }
    } );
    return true;
}
bool multiworld::save_subworld_manifest()
{
    if( MULTIWORLD.subworld_manifest.empty() ) {
        return true;
    }
    write_to_file( PATH_INFO::world_base_save_path_path() / "subworld_manifest.json", [&](
    std::ostream & fout ) {
        JsonOut jsout( fout );
        jsout.start_object();
        for( auto it = MULTIWORLD.subworld_manifest.begin(); it != MULTIWORLD.subworld_manifest.end();
             ++it ) {
            jsout.member( it->first );
            jsout.start_object();
            jsout.member( "region_type", it->second.region_type );
            jsout.member( "is_temporary", it->second.is_temporary );
            jsout.member( "parallel_world", it->second.parallel_world );
            jsout.end_object();
        }
        jsout.end_object();
    } );
    return true;
}