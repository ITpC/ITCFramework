#include <Config.h>
namespace itc
{
  using namespace reflection;

  /**
   * @brief constructor
   * 
   * @param fname a filename of config file to read the array from.
   **/
  Config::Config(const std::string& fname)
  {
    std::fstream fs(fname, std::ios_base::in);
    std::string input;
    while(fs.good())
    {
      char c;
      fs.get(c);
      input.append(1u, c);
    }
    utils::StringTokenizer aTokenizer;

    tokens = aTokenizer.scan(input, "", "\",{}: \t\n");
    parse();
    fs.close();
  };
}
