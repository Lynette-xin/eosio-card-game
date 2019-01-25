#include "gameplay.cpp"

void cardgame::login(account_name username) {
  //授权
  require_auth(username);

  // 如果表中不存在就把用户信息加入到表中
  auto user_iterator = _users.find(username);
  if (user_iterator == _users.end()) {
    user_iterator = _users.emplace(username,  [&](auto& new_user) {
      new_user.name = username;
    });
  }
}

void cardgame::startgame(account_name username) {
  // 同上
  require_auth(username);

  // "User doesn't exist" ，如果没找到用户就提示这个错误
  auto& user = _users.get(username, "User doesn't exist");

  _users.modify(user, username, [&](auto& modified_user) {
    //创建一个空的游戏数据，用于后面的存储
    game game_data;

    // 循环给每个人分配四张牌
    for (uint8_t i = 0; i < 4; i++) {
      //每次从总的17张牌中抽出一张放在手上
      draw_one_card(game_data.deck_player, game_data.hand_player);
      draw_one_card(game_data.deck_ai, game_data.hand_ai);
    }

    // 覆盖掉原来的数据
    modified_user.game_data = game_data;
  });
}

void cardgame::endgame(account_name username) {
  require_auth(username);

  auto& user = _users.get(username, "User doesn't exist");
  _users.modify(user, username, [&](auto& modified_user) {
    //游戏结束了，清空玩家的游戏数据
    modified_user.game_data = game();
  });
}

void cardgame::playcard(account_name username, uint8_t player_card_idx) {
  require_auth(username);

  // 选的牌存在？
  eosio_assert(player_card_idx < 4, "playcard: Invalid hand index");
  auto& user = _users.get(username, "User doesn't exist");

  // 游戏在进行？游戏中轮到你出牌了吗
  eosio_assert(user.game_data.status == ONGOING,
               "playcard: This game has ended. Please start a new one");
  eosio_assert(user.game_data.selected_card_player == 0,
               "playcard: The player has played his card this turn!");
  _users.modify(user, username, [&](auto& modified_user) {
    game& game_data = modified_user.game_data;

    // 获得用户选中的牌
    game_data.selected_card_player = game_data.hand_player[player_card_idx];
    game_data.hand_player[player_card_idx] = 0;

    // 机器选个牌
    int ai_card_idx = ai_choose_card(game_data);
    game_data.selected_card_ai = game_data.hand_ai[ai_card_idx];
    game_data.hand_ai[ai_card_idx] = 0;
    //选中的牌删掉，切换谁出牌
    resolve_selected_cards(game_data);
    update_game_status(modified_user);
  });
}

void cardgame::nextround(account_name username) {
  require_auth(username);
  auto& user = _users.get(username, "User doesn't exist");
  eosio_assert(user.game_data.status == ONGOING,
              "nextround: This game has ended. Please start a new one.");
  eosio_assert(user.game_data.selected_card_player != 0 && user.game_data.selected_card_ai != 0,
               "nextround: Please play a card first.");

  _users.modify(user, username, [&](auto& modified_user) {
    game& game_data = modified_user.game_data;

    //新的一轮了，把数据的置为初始值
    game_data.selected_card_player = 0;
    game_data.selected_card_ai = 0;
    game_data.life_lost_player = 0;
    game_data.life_lost_ai = 0;

    // 出牌
    if (game_data.deck_player.size() > 0) draw_one_card(game_data.deck_player, game_data.hand_player);
    if (game_data.deck_ai.size() > 0) draw_one_card(game_data.deck_ai, game_data.hand_ai);
  });
}


EOSIO_ABI(cardgame, (login)(startgame)(playcard)(nextround)(endgame))//在这里声明各个action，让前端的takeaction可以找的到
