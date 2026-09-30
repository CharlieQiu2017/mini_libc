extern "C" [[noreturn]] void exit_group (int ret);

namespace std {
  [[noreturn]] void terminate () noexcept {
    exit_group (1);
  }
}
