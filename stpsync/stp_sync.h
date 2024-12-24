/**
 * @file stp_sync.h
 * @brief Заголовочный файл для синхронизации данных STP с базой данных.
 *
 * Этот файл содержит определения класса `StpSync`, который отвечает за
 * управление синхронизацией данных STP с базами данных конфигурации
 * и состояния приложения.
 *
 * @details
 * - Файл предоставляет API для управления VLAN, портами, экземплярами STP.
 * - Обеспечивает функциональность синхронизации данных с базой данных
 *   конфигурации (Configuration Database) и базой данных состояния (State Database).
 *
 * Связанные файлы:
 * - @ref stp_sync.cpp
 * - @ref stpctl.h
 * - @ref stpctl.c
 *
 * @see https://github.com/sonic-net/sonic-stp
 * @copyright
 * Broadcom, 2019. Лицензия Apache License 2.0.
 *
 * @authors
 * Команда Broadcom
 */

#ifndef __STPSYNC__
#define __STPSYNC__

#include <string>
#include "dbconnector.h"
#include "producerstatetable.h"
#include "stp_dbsync.h"

namespace swss
{

    class StpSync
    {
    public:
        /**
         * @brief Конструктор класса.
         * @param db Указатель на объект подключения к базе данных состояния.
         * @param cfgDb Указатель на объект подключения к базе данных конфигурации.
         */
        StpSync(DBConnector *db, DBConnector *cfgDb);
        /**
         * @brief Добавляет VLAN в экземпляр STP.
         * @param vlan_id Идентификатор VLAN.
         * @param instance Идентификатор экземпляра STP.
         */
        void addVlanToInstance(uint16_t vlan_id, uint16_t instance);
        /**
         * @brief Удаляет VLAN из экземпляра STP.
         * @param vlan_id Идентификатор VLAN.
         * @param instance Идентификатор экземпляра STP.
         */
        void delVlanFromInstance(uint16_t vlan_id, uint16_t instance);
        /**
         * @brief Обновляет информацию о VLAN в STP.
         * @param stp_vlan Указатель на структуру с информацией о VLAN.
         */
        void updateStpVlanInfo(STP_VLAN_TABLE *stp_vlan);
        /**
         * @brief Удаляет информацию о VLAN в STP.
         * @param vlan_id Идентификатор VLAN.
         */
        void delStpVlanInfo(uint16_t vlan_id);
        /**
         * @brief Обновляет информацию о интерфейсах VLAN в STP.
         * @param stp_vlan_intf Указатель на структуру с информацией о интерфейсе VLAN.
         */
        void updateStpVlanInterfaceInfo(STP_VLAN_PORT_TABLE *stp_vlan_intf);
        /**
         * @brief Удаляет информацию о интерфейсе VLAN в STP.
         * @param if_name Имя интерфейса.
         * @param vlan_id Идентификатор VLAN.
         */
        void delStpVlanInterfaceInfo(char *if_name, uint16_t vlan_id);
        /**
         * @brief Обновляет состояние порта STP.
         * @param ifName Имя интерфейса.
         * @param instance Идентификатор экземпляра STP.
         * @param state Новое состояние порта.
         */
        void updateStpPortState(char *ifName, uint16_t instance, uint8_t state);
        /**
         * @brief Удаляет состояние порта STP.
         * @param ifName Имя интерфейса.
         * @param instance Идентификатор экземпляра STP.
         */
        void delStpPortState(char *ifName, uint16_t instance);
        /**
         * @brief Обновляет состояние порта VLAN в STP.
         * @param ifName Имя интерфейса.
         * @param vlan_id Идентификатор VLAN.
         * @param state Новое состояние порта.
         */
        void updateStpVlanPortState(char *ifName, uint16_t vlan_id, uint8_t state);
        /**
         * @brief Удаляет состояние порта VLAN в STP.
         * @param ifName Имя интерфейса.
         * @param vlan_id Идентификатор VLAN.
         */
        void delStpVlanPortState(char *ifName, uint16_t vlan_id);
        /**
         * @brief Обновляет информацию о быстром старении для VLAN.
         * @param vlan_id Идентификатор VLAN.
         * @param add Указывает, добавлять или удалять VLAN из режима быстрого старения.
         */
        void updateStpVlanFastage(uint16_t vlan_id, bool add);
        /**
         * @brief Обновляет административное состояние порта.
         * @param if_name Имя интерфейса.
         * @param up Указывает, должен ли порт быть включён.
         * @param physical Указывает, является ли порт физическим.
         */
        void updatePortAdminState(char *if_name, bool up, bool physical);
        /**
         * @brief Получает скорость порта.
         * @param if_name Имя интерфейса.
         * @return Скорость порта в Мбит/с.
         */
        uint32_t getPortSpeed(char *if_name);
        /**
         * @brief Обновляет состояние BPDU Guard для порта.
         * @param ifName Имя интерфейса.
         * @param disabled Указывает, отключён ли BPDU Guard.
         */
        void updateBpduGuardShutdown(char *ifName, bool disabled);
        /**
         * @brief Удаляет интерфейс из STP.
         * @param ifName Имя интерфейса.
         */
        void delStpInterface(char *ifName);
        /**
         * @brief Обновляет состояние функции Fast на порту.
         * @param ifName Имя интерфейса.
         * @param disabled Указывает, отключена ли функция Fast.
         */
        void updatePortFast(char *ifName, bool disabled);
        /**
         * @brief Очищает все таблицы приложения STP.
         */
        void clearAllStpAppDbTables(void);

    protected:
    private:
        ProducerStateTable m_stpVlanTable;         /**< Таблица состояния VLAN. */
        ProducerStateTable m_stpVlanPortTable;     /**< Таблица состояния портов VLAN. */
        ProducerStateTable m_stpVlanInstanceTable; /**< Таблица экземпляров VLAN. */
        ProducerStateTable m_stpPortTable;         /**< Таблица состояния портов. */
        ProducerStateTable m_stpPortStateTable;    /**< Таблица состояния портов STP. */
        ProducerStateTable m_appVlanMemberTable;   /**< Таблица членов VLAN. */
        ProducerStateTable m_stpFastAgeFlushTable; /**< Таблица для быстрого удаления записей. */
        Table m_appPortTable;                      /**< Таблица портов приложения. */
        Table m_cfgPortTable;                      /**< Таблица конфигурации портов. */
        Table m_cfgLagTable;                       /**< Таблица конфигурации агрегатов портов (LAG). */
    };

}

#endif
