#include <eosiolib/eosio.hpp>

using namespace std;
class cardgame : public eosio::contract {

  private:

    enum game_status: int8_t  {
      ONGOING     = 0,//用于进行判断游戏是否在进行中
      PLAYER_WON   = 1,
      PLAYER_LOST  = -1
    };

    enum card_type: uint8_t {
      EMPTY = 0, // 手中的牌为空
      FIRE = 1,
      WOOD = 2,
      WATER = 3,
      NEUTRAL = 4,
      VOID = 5
    };

    struct card {
      uint8_t type; //木、水、火、空、中
      uint8_t attack_point; //伤害值
    };

    typedef uint8_t card_id; //容易混淆，切记只是个类型，不是新定义的一个

    const map<card_id, card> card_dict = { //牌的几种类型搭配，不是所有的类型都有1-3的伤害值
      { 0, {EMPTY, 0} },
      { 1, {FIRE, 1} },
      { 2, {FIRE, 1} },
      { 3, {FIRE, 2} },
      { 4, {FIRE, 2} },
      { 5, {FIRE, 3} },
      { 6, {WOOD, 1} },
      { 7, {WOOD, 1} },
      { 8, {WOOD, 2} },
      { 9, {WOOD, 2} },
      { 10, {WOOD, 3} },
      { 11, {WATER, 1} },
      { 12, {WATER, 1} },
      { 13, {WATER, 2} },
      { 14, {WATER, 2} },
      { 15, {WATER, 3} },
      { 16, {NEUTRAL, 3} },
      { 17, {VOID, 0} }
    };


    struct game {
      int8_t          life_player = 5; //生命值都为5
      int8_t          life_ai = 5;
      //账号一创建他的手中具有这么多的牌
      vector<card_id> deck_player = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
      vector<card_id> deck_ai = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
      vector<card_id> hand_player = {0, 0, 0, 0};
      vector<card_id> hand_ai = {0, 0, 0, 0};
      card_id         selected_card_player = 0;
      card_id         selected_card_ai = 0;
      uint8_t         life_lost_player = 0;
      uint8_t         life_lost_ai = 0;
      int8_t          status = ONGOING;
    };

    // 
    struct user_info {
      account_name    name;
      uint16_t        win_count = 0;
      uint16_t        lost_count = 0;
      game            game_data;

      auto primary_key() const { return name; }　//作为用户表示的主键
    };

    // 用于生成随机数的一个特殊定义
    struct seed {
      uint64_t        key = 1;
      uint32_t        value = 1;

      auto primary_key() const { return key; }
    };

    typedef eosio::multi_index<N(users), user_info> users_table; //multi_index为eosio库中的数据表对象，这里是一个库函数，<表名，每行对象>
    //N()函数为把原来的字符转为base32编码的， 把他重命名为users_table，方便以后初始化时不用写那么长

    typedef eosio::multi_index<N(seed), seed> seed_table;

    users_table _users;

    seed_table _seed;

    void draw_one_card(vector<uint8_t>& deck, vector<uint8_t>& hand);

    int calculate_attack_point(const card& card1, const card& card2);

    int ai_best_card_win_strategy(const int ai_attack_point, const int player_attack_point);

    int ai_min_loss_strategy(const int ai_attack_point, const int player_attack_point);

    int ai_points_tally_strategy(const int ai_attack_point, const int player_attack_point);

    int ai_loss_prevention_strategy(const int8_t life_ai, const int ai_attack_point, const int player_attack_point);

    int calculate_ai_card_score(const int strategy_idx, const int8_t life_ai,
                                const card& ai_card, const vector<uint8_t> hand_player);

    int ai_choose_card(const game& game_data);

    void resolve_selected_cards(game& game_data);

    void update_game_status(user_info& user);

    int random(const int range);

  public:

    cardgame( account_name self ):contract(self),_users(self, self),_seed(self, self){}

    void login(account_name username);

    void startgame(account_name username);

    void endgame(account_name username);

    void playcard(account_name username, uint8_t player_card_idx);

    void nextround(account_name username);

};
