/**
 * @file simulate.hpp
 * @author Jingxuan Liu, Yufei li
 * @brief An integrated module for game simulation.
 * @date 2023-04-01
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include "game_info.hpp"
#include "control.hpp"

/**
 * @brief Enumerate values showing whether the game is running, and with detailed reasons
 * if the game ends.
 */
enum GameState
{
    Player0Win, ///< Game ends when player 0 wins the game.
    Player1Win, ///< Game ends when player 1 wins the game.
    Running,    ///< Game is still running.
    Undecided   ///< Game ends due to round limit exceeded. Further checking for the winner is needed.
};

/**
 * @brief An integrated module for simulation with simple interfaces for your convenience.
 * Built from the game state of a Controller instance, a Simulator object allows you to
 * simulate the whole game and "predict" the future for decision making.
 */
class Simulator
{
private:
    GameInfo info;                          ///< Game state
    std::vector<Operation> operations[2];   ///< Players' operations which are about to be applied to current game state. 

    /* Round settlement process */

    /**
     * @brief Lightning storms snd towers try attacking ants.
     * 
     * @note A tower may not attack if it has not cooled down (i.e. CD > 0) or if no target is available.
     * Even if it is able to attack, a tower may not cause any damage due to item effects.
     * 
     * @note The state of an ant may be changed. Set AntState::Fail if an ant has negative health points(hp).
     * Set AntState::Frozen if an ant is attacked by a tower of type TowerType::Ice.
     * 
     * @see #AntState for more information on the life cycle of an ant.
     */
    void attack_ants()
    {
        /* Lightning Storm Attack */

        for (SuperWeapon& sw: info.super_weapons)
        {
            if(sw.type != SuperWeaponType::LightningStorm)
                continue;
            for (Ant &ant : info.ants)
            {
                if (sw.is_in_range(ant.x, ant.y) 
                    && ant.player != sw.player)
                {
                    ant.hp = 0;
                    ant.state = AntState::Fail;
                    info.update_coin(sw.player, ant.reward());
                    info.antKills[sw.player]++;
                }
            }
        }
        
        /* Tower Attack */
        
        // Set deflector property
        for (Ant& ant: info.ants)
            ant.deflector = info.is_shielded_by_deflector(ant);
        // Attack
        for (Tower& tower: info.towers)
        {
            // Skip if shielded by EMP
            if (info.is_shielded_by_emp(tower))
                continue;
            // Try to attack
            auto targets = tower.attack(info.ants);
            // Get coins if tower killed the target
            for (int idx: targets)
            {
                if (info.ants[idx].state == AntState::Fail)
                    info.update_coin(tower.player, info.ants[idx].reward());
                    info.antKills[tower.player^1]++;
            }
            // Reset tower's damage (clear buff effect)
            tower.damage = TOWER_INFO[tower.type].attack;
        }
        // Reset deflector property
        for (Ant& ant: info.ants)
            ant.deflector = false;
    }

    /**
     * @brief Make alive ants move according to pheromone, without modifying pheromone. 
     * 
     * @note The state of an ant may be changed. Set AntState::TooOld if an ant reaches age limit.
     * Set AntState::Success if an ant has reached opponent's base, then update the base's health points (hp).   
     * 
     * @return Current game state (running / ended for one side's hp <= 0).
     * 
     * @see #AntState for more information on the life cycle of an ant.
     */
    GameState move_ants()
    {
        for (Ant& ant: info.ants)
        {
            // Update age regardless of the state
            ant.age++;
            // 1) No other action for dead ants
            if (ant.state == AntState::Fail)
                continue;
            // 2) Check if too old
            if (ant.age > Ant::AGE_LIMIT)
                ant.state = AntState::TooOld;
            // 3) Move if possible (alive)
            if (ant.state == AntState::Alive)
                ant.move(info.next_move(ant));
            // 4) Check if success (Mark success even if it reaches the age limit)
            if (ant.x == Base::POSITION[!ant.player][0] && ant.y == Base::POSITION[!ant.player][1])
            {
                ant.state = AntState::Success;
                info.update_base_hp(!ant.player, -1);
                info.update_coin(ant.player, 5);
                // If hp of one side's base reaches 0, game over 
                if (info.bases[!ant.player].hp <= 0)
                    return (ant.player == 0) ? GameState::Player0Win : GameState::Player1Win;
            }
            // 5) Unfreeze if frozen
            if (ant.state == AntState::Frozen)
                ant.state = AntState::Alive;
        }
        return GameState::Running;
    }

    /**
     * @brief Bases try generating new ants.
     * @note Generation may not happen if it is not the right time (i.e. round % cycle == 0).
     */
    void generate_ants()
    {
        for (auto& base: info.bases)
        {
            auto ant = base.generate_ant(info.next_ant_id, info.round);
            if (ant)
            {
                info.ants.push_back(std::move(ant.value()));
                info.next_ant_id++;
            }
        }
    }

    /**
     * @brief Get the basic income for a player and add it to corresponding coins.
     * @param player_id The player's ID.
     */
    void get_basic_income(int player_id)
    {
        info.update_coin(player_id, BASIC_INCOME);
    }

    /* Game judger */

    /**
     * @brief Judge winner at MAX_ROUND.
     * @return Game result.
     * @note The function returns GameState::Undecided when both players have the same hp.
     */
    GameState judge_winner() const
    {
        if (info.bases[0].hp < info.bases[1].hp)
            return GameState::Player1Win;
        else if (info.bases[0].hp > info.bases[1].hp)
            return GameState::Player0Win;
        else if (info.antKills[0]>info.antKills[1])
            return GameState::Player0Win;
        else if (info.antKills[0]<info.antKills[1])
            return GameState::Player1Win;
        else
            return GameState::Undecided;
    }

public:
    /**
     * @brief Construct a new Simulator object from a GameInfo instance. Current game state will be copied.
     * @param info The GaemInfo instance as data source.
     */
    Simulator(const GameInfo& info) : info(info) {}

    /**
     * @brief Get information about current game state.
     * @return A read-only (constant) reference to the current GameInfo object.
     */
    const GameInfo& get_info()
    {
        return info;
    }

    /**
     * @brief Get added operations of a player.
     * @param player_id The player.
     * @return A read-only (constant) reference to "operations[player_id]".
     */
    const std::vector<Operation>& get_operations_of_player(int player_id) const
    {
        return operations[player_id];
    }

    /**
     *  @brief Try adding an operation to "operations[player_id]". The operation has been constructed elsewhere.
     *         This function will check validness of the operation and add it to "operations[player_id]" if valid.  
     *  @param player_id The player.
     *  @param op The operation to be added.
     *  @return Whether the operation is added successfully.
     */
    bool add_operation_of_player(int player_id, Operation op)
    {
        assert(operations[player_id].empty());
        if (info.is_operation_valid(player_id, operations[player_id], op))
        {
            operations[player_id].push_back(op);
            return true;
        }
        return false;
    }

    /**
     * @brief Apply all operations in "operations[player_id]" to current state.
     * @param player_id The player.
     */
    void apply_operations_of_player(int player_id)
    {
        // 1) count down long-lasting weapons' left-time
        info.count_down_super_weapons_left_time(player_id);
        // 2) apply opponent's operations
        for (auto& op: operations[player_id])
            info.apply_operation(player_id, op);
    }

    /**
     * @brief Update game state at the end of current round.
     * This function is called after both players have applied their operations.
     * @return Current game state (running / ended with some reasons).
     */
    GameState next_round()
    {
        // 1) Judge winner at MAX_ROUND
        if (info.round == MAX_ROUND)
            return judge_winner();
        // 2) Towers attack ants
        attack_ants();
        // 3) Ants move
        GameState state = move_ants();
        if (state != GameState::Running)
            return state;
        // 4) Update pheromone
        info.global_pheromone_attenuation();
        info.update_pheromone_for_ants();
        // 5) Clear dead and succeeded ants
        info.clear_dead_and_succeeded_ants();
        // 6) Barracks generate new ants
        generate_ants();
        // 7) Get basic income
        get_basic_income(0);
        get_basic_income(1);
        // 8) Start next round
        info.round++;
        // 9) Count down super weapons' cd
        info.count_down_super_weapons_cd();
        // 10) Clear operations
        operations[0].clear();
        operations[1].clear();

        return GameState::Running;
    }
};