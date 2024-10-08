#pragma once
#ifndef CATA_SRC_MULTIWORLD_H
#define CATA_SRC_MULTIWORLD_H
//trans people are so cool :)

#include <climits>
#include <cstdint>
#include <iosfwd>
#include <new>
#include <optional>
#include <string>
#include <vector>

#include "game.h"
#include "calendar.h"
#include "catacharset.h"
#include "color.h"
#include "damage.h"
#include "translations.h"
#include "type_id.h"
#include "point.h"
class multiworld
{
        //allow access to set_world_prefix
        friend void game::unserialize( std::istream &fin, const cata_path &path );
        friend void game::setup();
    public:
        //struct for a subworld's settings
        /**@param is_temporary - delete world after travelling out of it
         * @param parallel_world - don't clear the overmap buffer when travelling into this world
         * @param region_type - which region blueprint to use when inside the world*/
        struct subworld_settings {
            bool is_temporary = false;
            bool parallel_world = false;
            std::string region_type = "default";
        };
        multiworld();
        ~multiworld();
        std::map<std::string, subworld_settings> subworld_manifest;
        bool load();
        bool load_subworld_manifest();
        bool save_subworld_manifest();
        std::string get_world_prefix();
        bool create_or_modify_world( const std::string &prefix );
        //currently just the player
        bool travel_to_world( const std::string &prefix );
        std::string get_world_region_type( const std::string &world_prefix );
        std::string get_current_world_region_type();
    private:
        /** @param world_prefix tells the game which world folder it should load data from
         *  @param set_world_prefix sets the current prefix, it shouldn't be used on it's own
        */
        std::string world_prefix;
        /*the game already loads all regions in the JSON data into 'region_settings_map', so I'm not bothering doing that here.*/
        void set_world_prefix( std::string prefix );
};


extern multiworld MULTIWORLD;
#endif // CATA_SRC_MULTIWORLD_H
