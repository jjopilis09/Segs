/*
 * SEGS - Super Entity Game Server
 * http://www.segs.io/
 * Copyright (c) 2006 - 2018 SEGS Team (see AUTHORS.md)
 * This software is licensed under the terms of the 3-clause BSD License. See LICENSE.md for details.
 */

#pragma once
#include "CommonNetStructures.h"
#include "Powers.h"
#include "Costume.h"
#include "Team.h"
#include "FixedPointValue.h"
#include "Common/GameData/entitydata_definitions.h"
#include "Common/GameData/chardata_definitions.h"
#include "Common/GameData/CoHMath.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/constants.hpp>

#include <QQueue>
#include <array>
#include <memory>

struct MapClientSession;
class Team;
class Trade;
class Character;
struct PlayerData;
class GameDataStore;
using Parse_AllKeyProfiles = std::vector<struct Keybind_Profiles>;

class PosUpdate
{
public:
    glm::vec3       m_position;
    AngleRadians    m_pyr_angles[3];
    int             m_timestamp;
};

class InputStateStorage
{
public:
    InputStateStorage()
    {
        for(int i=0; i<3; ++i)
        {
            pos_delta_valid[i]=false;
            pyr_valid[i]=false;
        }
    }

    uint8_t     m_csc_deltabits                 = 0;
    bool        m_send_deltas                   = false;
    uint16_t    m_control_bits                  = 0;
    uint16_t    m_send_id                       = 0;
    glm::vec3   m_camera_pyr;
    glm::vec3   m_orientation_pyr;              // Stored in Radians
    glm::quat   m_direction;
    int         m_time_diff1                    = 0;
    int         m_time_diff2                    = 0;
    uint8_t     m_input_vel_scale               = 0; // TODO: Should be float?
    uint8_t     m_received_server_update_id     = 0;
    bool        m_no_collision                  = false;
    bool        m_input_received                = false;
    bool        m_has_input_commit_guess        = false; // was has_input_commit_guess
    bool        pos_delta_valid[3]              = {};
    bool        pyr_valid[3]                    = {};
    glm::vec3   pos_delta;
    bool        m_controls_disabled             = false;

    InputStateStorage & operator=(const InputStateStorage &other);
    void processDirectionControl(int dir, int prev_time, int press_release);
    template<class Archive>
    void serialize(Archive &ar)
    {
        ar(m_csc_deltabits);
        ar(m_send_deltas);
        ar(m_control_bits);
        ar(m_send_id);
        ar(m_camera_pyr);
        ar(m_orientation_pyr);
        ar(m_direction);
        ar(m_time_diff1);
        ar(m_time_diff2);
        ar(m_input_vel_scale);
        ar(m_received_server_update_id);
        ar(m_no_collision);
        ar(m_has_input_commit_guess);
        ar(pos_delta_valid);
        ar(pyr_valid);
        ar(pos_delta);
        ar(m_controls_disabled);
    }
};

enum class FadeDirection
{
    In,
    Out
};

struct Destination // aka waypoint
{
    int point_idx = 0;
    glm::vec3 location;
};

// returned by getEntityFromDB()
struct CharacterFromDB
{
    QString         name;
    EntityData      entity_data;
    CharacterData   char_data;
    float           hitpoints;
    float           endurance;
    uint32_t        sg_id;
    uint32_t        m_db_id;
};

enum class EntType : uint8_t
{
    Invalid = 0,
    NPC     = 1,
    PLAYER  = 2,
    HERO    = 3,
    CRITTER = 4,
    CAR     = 5,
    DELIVERYTARGET = 6,
    MOBILEGEOMETRY = 7,
    MISSION_ITEM   = 8,
    MAPXFERDOOR    = 9,
    DOOR           = 10,
    COUNT          = 11
};

enum class AppearanceType : uint8_t
{
    None          = 0,
    WholeCostume  = 1,
    NpcCostume    = 2,
    VillainIndex  = 3,
    SequencerName = 4
};

struct SuperGroup
{
    int             m_SG_id         = {0};
    QString         m_SG_name       = "Supergroup"; // 64 chars max
    QString         m_SG_motto;
    QString         m_SG_motd;
    QString         m_SG_emblem;         // 128 chars max -> hash table key from the CostumeString_HTable
    uint32_t        m_SG_color1     = 0; // supergroup color 1
    uint32_t        m_SG_color2     = 0; // supergroup color 2
    int             m_SG_rank       = 1;
};

struct NPCData
{
    bool m_is_owned = false;
    const struct Parse_NPC *src_data;
    int npc_idx=0;
    int costume_variant=0;
};

struct NetFxTarget
{
    bool        type_is_location = false;
    uint32_t    ent_idx = 0;
    glm::vec3   pos;
};

struct NetFx
{
    uint8_t     command;
    uint32_t    net_id;
    uint32_t    handle;
    bool        pitch_to_target     = false;
    uint8_t     bone_id;
    float       client_timer        = 0;
    int         client_trigger_fx   = 0;
    float       duration            = 0;
    float       radius              = 0;
    QString     power               = 0;
    int         debris              = 0;
    NetFxTarget target;
    NetFxTarget origin;
};


class Entity
{
    // only EntityStore can create instances of this class
    friend class EntityStore;
    friend std::array<Entity,10240>;
    using CharacterPtr = std::unique_ptr<Character>;
    using PlayerPtr = std::unique_ptr<PlayerData>;
    using EntityPtr = std::unique_ptr<EntityData>;
    using NPCPtr = std::unique_ptr<NPCData>;
    using TradePtr = std::shared_ptr<Trade>;
private:
                            Entity();
                            ~Entity();
public:
        struct currentInputState
        {
            glm::vec3 pos;
            glm::vec3 pyr; //TODO: convert to quat
        };
        InputStateStorage   inp_state;
        // Some entities might not have a character data ( doors, cars )
        // Making it an unique_ptr<Character> makes it clear that Entity 'owns'
        // and takes care of this data, at the same time it can be missing
        CharacterPtr        m_char;
        // And not all entities are players
        PlayerPtr           m_player;
        EntityPtr           m_entity;
        NPCPtr              m_npc;

        int                 m_full_update_count     = 10; // TODO: remove this after we have proper physics
        bool                m_has_supergroup        = true;
        SuperGroup          m_supergroup;                       // client has this in entity class, but maybe move to Character class?
        bool                m_has_team              = false;
        Team *              m_team                  = nullptr;  // we might want t move this to Character class, but maybe Baddies use teams?
        TradePtr            m_trade;
        EntityData          m_entity_data;

        uint32_t            m_idx                   = {0};
        uint32_t            m_db_id                 = {0};
        EntType             m_type                  = {EntType::Invalid};
        glm::quat           m_direction;
        glm::vec3           m_spd                   = {1,1,1};
        int32_t             m_target_idx            = -1;
        int32_t             m_assist_target_idx     = -1;

        std::vector<Buffs>          m_buffs;
        QQueue<QueuedPowers>        m_queued_powers;
        std::vector<QueuedPowers>   m_recharging_powers;
        std::vector<NetFx>          m_net_fx;
        CharacterPower    * m_stance                = nullptr;
        bool                m_update_buffs          = false;

        int                 m_randSeed              = 0;    // Sequencer uses this as a seed for random bone scale
        int                 m_num_fx                = 0;
        bool                m_is_logging_out        = false;
        int                 m_time_till_logout      = 0;    // time in miliseconds untill given entity should be marked as logged out.
        AppearanceType      m_costume_type          = AppearanceType::None;
        int                 m_state_mode            = 0;
        bool                m_has_state_mode       = false;
        bool                m_odd_send              = false;
        bool                m_no_draw_on_client     = false;
        bool                m_seq_update            = false;
        bool                m_force_camera_dir      = false; // used to force the client camera direction in sendClientData()
        bool                m_is_hero               = false;
        bool                m_is_villian            = false;
        bool                m_contact               = false;
        int                 m_seq_upd_num1          = 0;
        int                 m_seq_upd_num2          = 0;
        bool                m_is_flying             = false;
        bool                m_is_stunned            = false;
        bool                m_is_jumping            = false;
        bool                m_is_sliding            = false;
        bool                m_is_falling            = false;
        bool                m_has_jumppack          = false;
        bool                m_controls_disabled     = false;
        float               m_backup_spd            = 1.0f;
        float               m_jump_height           = 2.0f;
        uint8_t             m_update_id             = 1;
        bool                m_update_part_1         = true;     // EntityResponse sendServerControlState
        bool                m_force_pos_and_cam     = true;     // EntityResponse sendServerControlState
        bool                m_full_update           = false;    // EntityReponse sendServerPhysicsPositions
        bool                m_has_control_id        = false;    // EntityReponse sendServerPhysicsPositions
        bool                m_extra_info            = false;    // EntityUpdateCodec storePosUpdate
        bool                m_move_instantly        = false;    // EntityUpdateCodec storePosUpdate
        bool                m_in_training           = false;

        bool                m_has_input_on_timeframe= false;

        int                 u1 = 0; // used for live-debugging

        PosUpdate           m_pos_updates[64];
        std::vector<PosUpdate> interpResults;
        size_t              m_update_idx                = 0;
        glm::vec3           m_velocity;
        glm::vec3           m_prev_pos;
        Vector3_FPV         fixedpoint_pos;
        bool                m_pchar_things              = false;
        bool                might_have_rare             = false;
        bool                m_hasname                   = false;
        bool                m_classname_override        = false;
        bool                m_hasRagdoll                = false;
        bool                m_has_owner                 = false;
        bool                m_create_player             = false;
        bool                m_rare_bits                 = false;
        int                 current_client_packet_id    = {0};
        QString             m_override_name;
        uint32_t            m_input_ack                 = {0};
        bool                player_type                 = false;
        bool                m_destroyed                 = false;
        uint32_t            ownerEntityId               = 0;
        uint32_t            creatorEntityId             = 0;
        float               translucency                = 1.0f;
        bool                m_is_fading                 = true;
        MapClientSession *  m_client                    = nullptr;
        FadeDirection       m_fading_direction          = FadeDirection::In;
        uint32_t            m_db_store_flags            = 0;
        Destination         m_cur_destination;

        void                dump();
        void                addPosUpdate(const PosUpdate &p);
        void                addInterp(const PosUpdate &p);

static  void                sendAllyID(BitStream &bs);
static  void                sendPvP(BitStream &bs);

        bool                update_rot(int axis) const; // returns true if given axis needs updating;

        const QString &     name() const;
        void                fillFromCharacter(const GameDataStore &data);
        void                beginLogout(uint16_t time_till_logout=10); // Default logout time is 10 s
};

enum class DbStoreFlags : uint32_t
{
    PlayerData = 1,
    Full       = ~0U,
};

void markEntityForDbStore(Entity *e,DbStoreFlags f);
void unmarkEntityForDbStore(Entity *e, DbStoreFlags f);
void initializeNewPlayerEntity(Entity &e);
void initializeNewNpcEntity(const GameDataStore &data, Entity &e, const Parse_NPC *src, int idx, int variant);
void fillEntityFromNewCharData(Entity &e, BitStream &src, const GameDataStore &data);
void forcePosition(Entity &e,glm::vec3 pos);
extern void abortLogout(Entity *e);
