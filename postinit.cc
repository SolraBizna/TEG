#include "postinit.hh"

#include <forward_list>
#include <vector>

typedef std::function<void()> Handler;

#if DEBUG
static bool postinit_done = false;
#endif

static std::pair<std::vector<std::forward_list<Handler>>,
                 std::vector<std::forward_list<Handler>>>& get_big_list() {
  static std::pair<std::vector<std::forward_list<Handler>>,
                   std::vector<std::forward_list<Handler>>> ret;
  return ret;
}

static std::forward_list<Handler>& get_handler_list(int priority) {
  auto& big_list = get_big_list();
  std::vector<std::forward_list<Handler>>* little_list;
  if(priority < 0) {
    priority = -priority - 1;
    little_list = &big_list.second;
  }
  else little_list = &big_list.first;
  if((unsigned)priority >= little_list->size())
    little_list->resize(priority + 1);
  return (*little_list)[priority];
}

TEG::PostInitHandler::PostInitHandler(int pri, std::function<void()> handler) {
#if DEBUG
  if(postinit_done) die("PostInitHandler registered too late!");
#endif
  get_handler_list(pri).emplace_front(handler);
}

void TEG::DoPostInit() {
#if DEBUG
  if(postinit_done) die("DoPostInit called more than once!");
  else postinit_done = true;
#endif
  auto& big_list = get_big_list();
  for(auto it = big_list.first.cbegin(); it != big_list.first.cend(); ++it) {
    for(auto sit : *it) 
      sit();
  }
  for(auto it = big_list.first.crbegin(); it != big_list.first.crend(); ++it) {
    for(auto sit : *it) 
      sit();
  }
}
