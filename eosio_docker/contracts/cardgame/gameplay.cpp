#include "cardgame.hpp"

// 伪随机算法，不建议使用，随机选个数字
int cardgame::random(const int range) {
  //把seed表中的头，赋给这个迭代器
  auto seed_iterator = _seed.begin();

  //如果为空，赋初始值
  if (seed_iterator == _seed.end()) {
    seed_iterator = _seed.emplace( _self, [&]( auto& seed ) { });
  }

  //生成新的seed值，他的一种计算方式
  int prime = 65537;
  auto new_seed_value = (seed_iterator->value + now()) % prime;

  _seed.modify( seed_iterator, _self, [&]( auto& s ) {
    s.value = new_seed_value;
  });

  //之所以还要模一下，是为控制这个seed在制定的范围内，总牌数嘛
  int random_result = new_seed_value % range;
  return random_result;
}

// 从池里选牌到手上去，vector为向量，不同于数组在于可以变长
void cardgame::draw_one_card(vector<uint8_t>& deck, vector<uint8_t>& hand) {
  //调用上面的随机算法选牌
  int deck_card_idx = random(deck.size());

  // 这里体现empty的作用
  int first_empty_slot = -1;
  for (int i = 0; i <= hand.size(); i++) {
    auto id = hand[i];
    if (card_dict.at(id).type == EMPTY) {
      first_empty_slot = i;
      break;
    }
  }
  eosio_assert(first_empty_slot != -1, "No empty slot in the player's hand");//手上的四张牌满了

  //赋值
  hand[first_empty_slot] = deck[deck_card_idx];
  // 从池里面移除掉Remove the card from the deck
  deck.erase(deck.begin() + deck_card_idx);
}

// 计算伤害值，考虑特殊情况
int cardgame::calculate_attack_point(const card& card1, const card& card2) {
  int result = card1.attack_point;

  //相冲，比如火克木，伤害值加一
  if ((card1.type == FIRE && card2.type == WOOD) ||
      (card1.type == WOOD && card2.type == WATER) ||
      (card1.type == WATER && card2.type == FIRE)) {
    result++;
  }

  return result;
}

//一系列的机器人他的出牌的算法，保证能让他有最大的几率能赢这个比赛
// 策略一,选择一张最有可能获胜的牌
int cardgame::ai_best_card_win_strategy(const int ai_attack_point, const int player_attack_point) {
  eosio::print("Best Card Wins");
  if (ai_attack_point > player_attack_point) return 3;
  if (ai_attack_point < player_attack_point) return -2;
  return -1; //相同情况
}

// 策略二,最大限度地减少损失
int cardgame::ai_min_loss_strategy(const int ai_attack_point, const int player_attack_point) {
  eosio::print("Minimum Losses");
  if (ai_attack_point > player_attack_point) return 1;
  if (ai_attack_point < player_attack_point) return -4;
  return -1;
}

// 策略三,选择造成最大伤害的牌
int cardgame::ai_points_tally_strategy(const int ai_attack_point, const int player_attack_point) {
  eosio::print("Points Tally");
  return ai_attack_point - player_attack_point;
}

// 策略四,出一个最可能延迟游戏时间的牌，让游戏不会马上结束。当AI剩余大量生命值时，这个策略不适用。所以，只有当AI剩余小于或等于2 生命值时才会选择这个策略
int cardgame::ai_loss_prevention_strategy(const int8_t life_ai, const int ai_attack_point, const int player_attack_point) {
  eosio::print("Loss Prevention");
  if (life_ai + ai_attack_point - player_attack_point > 0) return 1;
  return 0;
}

// 根据ai的给定策略和玩家手牌，计算分数
int cardgame::calculate_ai_card_score(const int strategy_idx,
                                      const int8_t life_ai,
                                      const card& ai_card,
                                      const vector<uint8_t> hand_player) {
   int card_score = 0;
   //和玩家手中的所以的牌进行比较，计算ai的这张的得分
   for (int i = 0; i < hand_player.size(); i++) {
      const auto player_card_id = hand_player[i];
      const auto player_card = card_dict.at(player_card_id);

      int ai_attack_point = calculate_attack_point(ai_card, player_card);
      int player_attack_point = calculate_attack_point(player_card, ai_card);

      // 根据给定的策略计算牌得分
      switch (strategy_idx) {
        case 0: {
          card_score += ai_best_card_win_strategy(ai_attack_point, player_attack_point);
          break;
        }
        case 1: {
          card_score += ai_min_loss_strategy(ai_attack_point, player_attack_point);
          break;
        }
        case 2: {
          card_score += ai_points_tally_strategy(ai_attack_point, player_attack_point);
          break;
        }
        default: {
          card_score += ai_loss_prevention_strategy(life_ai, ai_attack_point, player_attack_point);
          break;
        }
      }
    }
    return card_score;
}

// 从机器人手中选个牌
int cardgame::ai_choose_card(const game& game_data) {
  // 第四中为默认策略
  int available_strategies = 4;
  if (game_data.life_ai > 2) available_strategies--;
  //随机选择一个策略
  int strategy_idx = random(available_strategies);

  // 计算AI手中的牌的每张牌的得分，选出最高的那个
  int chosen_card_idx = -1;
  int chosen_card_score = std::numeric_limits<int>::min(); 

  for (int i = 0; i < game_data.hand_ai.size(); i++) {
    const auto ai_card_id = game_data.hand_ai[i];
    const auto ai_card = card_dict.at(ai_card_id);

    if (ai_card.type == EMPTY) continue;

    // 计算此AI牌相对于玩家牌的分数
    auto card_score = calculate_ai_card_score(strategy_idx, game_data.life_ai, ai_card, game_data.hand_player);

    // 记录得分最高的那张牌
    if (card_score > chosen_card_score) {
      chosen_card_score = card_score;
      chosen_card_idx = i;
    }
  }
  return chosen_card_idx;
}

//处理伤害
void cardgame::resolve_selected_cards(game& game_data) {
  const auto player_card = card_dict.at(game_data.selected_card_player);
  const auto ai_card = card_dict.at(game_data.selected_card_ai);

  // 体现void的作用了，清空，不就算伤害
  if (player_card.type == VOID || ai_card.type == VOID) return;
  //分别计算伤害值，要计算两次的原因是取决于，calculate_attack_point函数
  int player_attack_point = calculate_attack_point(player_card, ai_card);
  int ai_attack_point =  calculate_attack_point(ai_card, player_card);

  // 谁大，就减谁生命值
  if (player_attack_point > ai_attack_point) {
    int diff = player_attack_point - ai_attack_point;
    game_data.life_lost_ai = diff;
    game_data.life_ai -= diff;
  } else if (ai_attack_point > player_attack_point) {
    int diff = ai_attack_point - player_attack_point;
    game_data.life_lost_player = diff;
    game_data.life_player -= diff;
  }
}

//结束游戏的几种可能， 1.玩家或者机器人的生命值小于等于0 2.任何一方打完了牌
void cardgame::update_game_status(user_info& user) {
  game& game_data = user.game_data;

  if (game_data.life_ai <= 0) {
    game_data.status = PLAYER_WON;
  } else if (game_data.life_player <= 0) {
    game_data.status = PLAYER_LOST;
  } else {
    
    //机器人或者玩家打完牌了？
    const auto is_empty_slot = [&](const auto& id) { return card_dict.at(id).type == EMPTY; };
    bool player_finished = std::all_of(game_data.hand_player.begin(), game_data.hand_player.end(), is_empty_slot);
    bool ai_finished = std::all_of(game_data.hand_ai.begin(), game_data.hand_ai.end(), is_empty_slot);

    // 玩家或者机器人打完牌了，游戏结束
    if (player_finished || ai_finished) {
      if (game_data.life_player > game_data.life_ai) {
        game_data.status = PLAYER_WON;
      } else {
        game_data.status = PLAYER_LOST;
      }
    }
  }

  //更新数据
  if (game_data.status == PLAYER_WON) {
    user.win_count++;
  } else if (game_data.status == PLAYER_LOST) {
    user.lost_count++;
  }
}
