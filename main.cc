#include <string>
#include <string_view>
#include <dirent.h>

#include "generator.hh"

bool is_special(struct dirent const &entry)
{
  return entry.d_name == std::string(".") || entry.d_name == std::string("..");
}

/// We yield all `dirent`s contained in the particular `path` (as long as it is a directory)
inline generator<struct dirent> directory_entries(std::string_view path)
{
  DIR *dir_handle = opendir(path.data());

  while (auto dir_entry = readdir(dir_handle))
  {
    if (is_special(*dir_entry)) {
      continue;
    }

    co_yield *dir_entry;
  }

  closedir(dir_handle);
}

/// It often is useful to keep track of the _full_ path rather than only the entry's own name
/// So we wrap above generator with one that returns the full path for each dirent.
inline generator<std::pair<struct dirent, std::string_view>> directory_entries_with_paths(std::string_view path)
{
  for (auto const &entry : directory_entries(path))
  {
    auto full_path = std::string{path} + '/' + entry.d_name;
    co_yield std::make_pair(entry, full_path);
  }
}

/// We now have enough info to recurse through the directory tree:
///
/// We yield all entries in the current directory
/// and whenever we find that one of these entries is itself a directory,
/// we recurse with this new directory path.
inline generator<std::pair<struct dirent, std::string_view>> recursive_directory_entries(std::string_view path)
{
  for (auto const &entry_pair : directory_entries_with_paths(path))
  {
    co_yield entry_pair;

    auto [entry, entry_path] = entry_pair;
    if(entry.d_type == DT_DIR)
    {
      co_yield recursive_directory_entries(entry_path);
    }
  }
}

/// Run this program with a directory path as single argument
/// (it defaults to `.`, i.e. the current directory).
///
/// It will list all files and folders contained in that directory and all of its subdirectories.
int main(int argc, char **argv)
{
  auto path = argc == 1 ? "." : argv[1];

  for (auto [entry, entry_path ] : recursive_directory_entries(path))
  {
    std::cout << entry_path << '\n';
  }
}
