/*
 * SEGS - Super Entity Game Server
 * http://www.segs.io/
 * Copyright (c) 2006 - 2019 SEGS Team (see AUTHORS.md)
 * This software is licensed under the terms of the 3-clause BSD License. See LICENSE.md for details.
 */

#pragma once

#include "EntityStorage.h"
#include "EventProcessor.h"
#include "Common/Servers/ClientManager.h"
#include "Servers/ServerEndpoint.h"
#include "Servers/GameDatabase/GameDBSyncService.h"
#include "ScriptingEngine.h"
#include "MapClientSession.h"
#include "NpcGenerator.h"
#include "CritterGenerator.h"

#include "GameServer/EmailService/EmailService.h"
#include "GameServer/ClientOptionService/ClientOptionService.h"
#include "GameServer/CharacterService/CharacterService.h"
#include "GameServer/CharacterService/EnhancementService/EnhancementService.h"
#include "GameServer/CharacterService/InspirationService/InspirationService.h"
#include "GameServer/CharacterService/PowerService/PowerService.h"

#include <map>
#include <memory>
#include <vector>

#define WORLD_UPDATE_TICKS_PER_SECOND 30

class MapServer;
class SEGSTimer;
class World;
class GameDataStore;
class MapSceneGraph;
class MapXferData;

struct LuaTimer
{
    uint32_t                        m_entity_idx;
    bool                            m_is_enabled        = false;
    bool                            m_remove            = false;
    int64_t                         m_start_time;
    std::function<void(int64_t, int64_t, int64_t)>    m_on_tick_callback;

};

namespace SEGSEvents
{
class RecvInputState;
class Idle;
class DisconnectRequest;
class SceneRequest;
class EntitiesRequest;
class NewEntity;
class ShortcutsRequest;
class CookieRequest;
class WindowState;
class ConsoleCommand;
class ClientQuit;
class ConnectRequest;
class ChatDividerMoved;
class MiniMapState;
class ClientResumedRendering;
class LocationVisited;
class PlaqueVisited;
class EnterDoor;
class ChangeStance;
class SetDestination;
class HasEnteredDoor;
class DescriptionAndBattleCry;
class EntityInfoRequest;
class ChatReconfigure;
class UnqueueAll;
class TargetChatChannelSelected;
class SwitchTray;
class InteractWithEntity;
class RecvSelectedTitles;
class DialogButton;
class MapXferComplete;
class InitiateMapXfer;
struct ClientMapXferMessage;
struct MapSwapCollisionMessage;
class AwaitingDeadNoGurney;
class BrowserClose;
class LevelUpResponse;
class TradeWasCancelledMessage;
class TradeWasUpdatedMessage;
class RecvCostumeChange;
class DeadNoGurneyOK;
class ReceiveContactStatus;
class ReceiveTaskDetailRequest;
class SouvenirDetailRequest;
class StoreSellItem;
class StoreBuyItem;

// server<-> server event types
struct ExpectMapClientRequest;
struct WouldNameDuplicateResponse;
struct CreateNewCharacterResponse;
struct GetEntityResponse;
struct GetEntityByNameResponse;
class Timeout;
}
class MapLinkEndpoint;

class MapInstance final : public EventProcessor
{
        using SessionStore = ClientSessionStore<MapClientSession>;
        using ScriptEnginePtr = std::unique_ptr<ScriptingEngine>;
        QString                m_data_path;
        QMultiHash<QString, glm::mat4>  m_all_spawners;
        std::unique_ptr<SEGSTimer> m_world_update_timer;
        std::unique_ptr<SEGSTimer> m_resend_timer;
        std::unique_ptr<SEGSTimer> m_link_timer;
        std::unique_ptr<SEGSTimer> m_sync_service_timer;
        std::unique_ptr<SEGSTimer> m_afk_update_timer;
        std::unique_ptr<SEGSTimer> m_lua_timer;
        World *                 m_world;
        uint32_t                m_owner_id;
        uint32_t                m_instance_id;
        uint32_t                m_index = 1; // what does client expect this to store, and where do we send it?
        uint8_t                 m_game_server_id=255; // 255 is `invalid` id

        std::unique_ptr<GameDBSyncService>      m_sync_service;
        std::unique_ptr<EmailService>           m_email_service;
        std::unique_ptr<ClientOptionService>    m_client_option_service;
        std::unique_ptr<CharacterService>       m_character_service;
        std::unique_ptr<EnhancementService>     m_enhancement_service;
        std::unique_ptr<InspirationService>     m_inspiration_service;
        std::unique_ptr<PowerService>           m_power_service;

        // I think there's probably a better way to do this..
        // We load all transfers for the map to map_transfers, then on first access to zones or doors, we
        // then copy the relevant transfers to another hash which is then used for those specific transfers.
        // This means we only need to traverse the scenegraph once to get all transfers, but need to copy once
        // as well, rather than having to walk the scenegraph twice (once for each type).
        QHash<QString, MapXferData> m_map_transfers;
        QHash<QString, MapXferData> m_map_door_transfers;
        bool                    m_door_transfers_checked = false;
        QHash<QString, MapXferData> m_map_zone_transfers;
        bool                    m_zone_transfers_checked = false;

public:
        SessionStore            m_session_store;
        EntityManager           m_entities;
        ScriptEnginePtr         m_scripting_interface;
        MapLinkEndpoint *       m_endpoint = nullptr;
        ListenAndLocationAddresses m_addresses; //! this value is sent to the clients
        MapSceneGraph *         m_map_scenegraph;
        NpcGeneratorStore       m_npc_generators;
        CritterGeneratorStore   m_critter_generators;
        SpawnDefinitions        m_enemy_spawn_definitions;
        std::vector<LuaTimer>   m_lua_timers;

public:
                                IMPL_ID(MapInstance)
                                MapInstance(const QString &name,const ListenAndLocationAddresses &listen_addr);
                                ~MapInstance() override;
        void                    dispatch(SEGSEvents::Event *ev) override;

        const QString &         name() const { return m_data_path; }
        void                    load_map_lua();
        bool                    spin_up_for(uint8_t game_server_id, uint32_t owner_id, uint32_t instance_id);
        void                    start(const QString &scenegraph_path);
        void                    setPlayerSpawn(Entity &e);
        void                    setSpawnLocation(Entity &e, const QString &spawnLocation);
        glm::vec3               closest_safe_location(glm::vec3 v) const;
        QMultiHash<QString, glm::mat4> getSpawners() const { return m_all_spawners; }
        QHash<QString, MapXferData> get_map_door_transfers();
        QHash<QString, MapXferData> get_map_zone_transfers();
        QString                 getNearestDoor(glm::vec3 location);

        void send_player_update(Entity *e);
        void                    add_chat_message(Entity *sender, QString &msg_text);
        void                    startTimer(uint32_t entity_idx);
        void                    stopTimer(uint32_t entity_idx);
        void                    clearTimer(uint32_t entity_idx);

protected:
        // EventProcessor interface
        void                    serialize_from(std::istream &is) override;
        void                    serialize_to(std::ostream &is) override;

        void                    enqueue_client(MapClientSession *clnt);
        void                    spin_down();
        void                    init_timers();
        void                    init_services();
        uint32_t                index() const { return m_index; }
        void                    reap_stale_links();
        void                    on_client_connected_to_other_server(SEGSEvents::ClientConnectedMessage *ev);
        void                    on_client_disconnected_from_other_server(SEGSEvents::ClientDisconnectedMessage *ev);
        void                    process_chat(Entity *sender, QString &msg_text);

        // DB -> Server messages
        void                    on_name_clash_check_result(SEGSEvents::WouldNameDuplicateResponse *ev);
        void                    on_character_created(SEGSEvents::CreateNewCharacterResponse *ev);
        void                    on_entity_response(SEGSEvents::GetEntityResponse *ev);
        void                    on_entity_by_name_response(SEGSEvents::GetEntityByNameResponse *ev);
        // Server->Server messages
        void on_expect_client(SEGSEvents::ExpectMapClientRequest *ev);
        void on_expect_client_response(SEGSEvents::ExpectMapClientResponse *ev);

        void on_initiate_map_transfer(SEGSEvents::InitiateMapXfer *ev);
        void on_map_xfer_complete(SEGSEvents::MapXferComplete *ev);
        void on_map_swap_collision(SEGSEvents::MapSwapCollisionMessage *ev);

        void on_link_lost(SEGSEvents::Event *ev);
        void on_disconnect(SEGSEvents::DisconnectRequest *ev);
        void on_scene_request(SEGSEvents::SceneRequest *ev);
        void on_entities_request(SEGSEvents::EntitiesRequest *ev);
        void on_create_map_entity(SEGSEvents::NewEntity *ev);
        void on_timeout(SEGSEvents::Timeout *ev);
        void on_input_state(SEGSEvents::RecvInputState *st);
        void on_idle(SEGSEvents::Idle *ev);
        void on_shortcuts_request(SEGSEvents::ShortcutsRequest *ev);

        void sendState();
        void on_check_links();
        void on_update_entities();
        void on_afk_update();
        void on_lua_update();
        void send_character_update(Entity *e);

        void on_cookie_confirm(SEGSEvents::CookieRequest *ev);
        void on_window_state(SEGSEvents::WindowState *ev);
        void on_console_command(SEGSEvents::ConsoleCommand *ev);
        void on_client_quit(SEGSEvents::ClientQuit *ev);
        void on_connection_request(SEGSEvents::ConnectRequest *ev);
        void on_command_chat_divider_moved(SEGSEvents::ChatDividerMoved *ev);
        void on_minimap_state(SEGSEvents::MiniMapState *ev);
        void on_client_resumed(SEGSEvents::ClientResumedRendering *ev);
        void on_location_visited(SEGSEvents::LocationVisited *ev);
        void on_plaque_visited(SEGSEvents::PlaqueVisited *ev);
        void on_enter_door(SEGSEvents::EnterDoor *ev);
        void on_change_stance(SEGSEvents::ChangeStance *ev);
        void on_set_destination(SEGSEvents::SetDestination *ev);
        void on_has_entered_door(SEGSEvents::HasEnteredDoor *ev);
        void on_description_and_battlecry(SEGSEvents::DescriptionAndBattleCry *ev);
        void on_entity_info_request(SEGSEvents::EntityInfoRequest *ev);
        void on_chat_reconfigured(SEGSEvents::ChatReconfigure *ev);

        void on_unqueue_all(SEGSEvents::UnqueueAll *ev);
        void on_target_chat_channel_selected(SEGSEvents::TargetChatChannelSelected *ev);

        void on_emote_command(const QString &command, Entity *ent);
        void on_interact_with(SEGSEvents::InteractWithEntity *ev);
        void on_recv_selected_titles(SEGSEvents::RecvSelectedTitles *ev);
        void on_dialog_button(SEGSEvents::DialogButton *ev);

        void on_awaiting_dead_no_gurney(SEGSEvents::AwaitingDeadNoGurney *ev);
        void on_dead_no_gurney_ok(SEGSEvents::DeadNoGurneyOK *ev);
        void on_browser_close(SEGSEvents::BrowserClose *ev);
        void on_recv_costume_change(SEGSEvents::RecvCostumeChange *ev);

        void on_trade_cancelled(SEGSEvents::TradeWasCancelledMessage* ev);
        void on_trade_updated(SEGSEvents::TradeWasUpdatedMessage* ev);
        void on_receive_contact_status(SEGSEvents::ReceiveContactStatus *ev);
        void on_receive_task_detail_request(SEGSEvents::ReceiveTaskDetailRequest *ev);
        void on_souvenir_detail_request(SEGSEvents::SouvenirDetailRequest* ev);
        void on_store_sell_item(SEGSEvents::StoreSellItem* ev);
        void on_store_buy_item(SEGSEvents::StoreBuyItem* ev);

        void on_internal_service_to_client_response(SEGSEvents::InternalServiceToClientData* ev);
        void on_internal_service_to_client_response(std::vector<SEGSEvents::InternalServiceToClientData*> events);
        void on_map_service_to_client_response(SEGSEvents::MapServiceToClientData* ev);
        void on_map_service_to_client_response(std::vector<SEGSEvents::MapServiceToClientData*> events);
};
