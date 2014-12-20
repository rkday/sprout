#include "memcachedstore.h"
#include <cassert>

bool test_scaling(std::string unique_name,
                  std::vector<std::string> orig_servers,
                  std::vector<std::string> new_servers)
{
  std::vector<std::string> test_words = {"10-point", "10th", "11-point", "12-point", "16-point", "18-point", "1st", "2", "20-point", "2,4,5-t", "2,4-d", "2D", "2nd", "30-30", "3-D", "3-d", "3D", "3M", "3rd", "48-point", "4-D", "4GL", "4H", "4th", "5-point", "5-T", "5th", "6-point", "6th", "7-point", "7th", "8-point", "8th", "9-point", "9th", "-a", "A", "A.", "a", "a'", "a-", "a.", "A-1", "A1", "a1", "A4", "A5", "AA", "aa", "A.A.A.", "AAA", "aaa", "AAAA", "AAAAAA", "AAAL", "AAAS", "Aaberg", "Aachen", "AAE", "AAEE", "AAF", "AAG", "aah", "aahed", "aahing", "aahs", "AAII", "aal", "Aalborg", "Aalesund", "aalii", "aaliis", "aals", "Aalst", "Aalto", "AAM", "aam", "AAMSI", "Aandahl", "A-and-R", "Aani", "AAO", "AAP", "AAPSS", "Aaqbiye", "Aar", "Aara", "Aarau", "AARC", "aardvark", "aardvarks", "aardwolf", "aardwolves", "Aaren", "Aargau", "aargh", "Aarhus", "Aarika", "Aaron", "aaron", "Aaronic", "aaronic", "Aaronical", "Aaronite", "Aaronitic", "Aaron's-beard", "Aaronsburg", "Aaronson", "AARP", "aarrgh", "aarrghh", "Aaru", "AAS", "aas", "A'asia", "aasvogel", "aasvogels", "AAU", "AAUP", "AAUW", "AAVSO", "AAX", "A-axes", "A-axis", "A.B.", "AB", "Ab", "ab", "ab-", "A.B.A.", "ABA", "Aba", "aba", "Ababa", "Ababdeh", "Ababua", "abac", "abaca", "abacas", "abacate", "abacaxi", "abacay", "abaci", "abacinate", "abacination", "abacisci", "abaciscus", "abacist", "aback", "abacli", "Abaco", "abacot", "abacterial", "abactinal", "abactinally", "abaction"};
  std::vector<std::string> test_words_2 = {"abactor", "abaculi", "abaculus", "abacus", "abacuses", "Abad", "abada", "Abadan", "Abaddon", "abaddon", "abadejo", "abadengo", "abadia", "Abadite", "abaff", "abaft", "Abagael", "Abagail", "Abagtha", "Abailard", "abaisance", "abaised", "abaiser", "abaisse", "abaissed", "abaka", "Abakan", "abakas", "Abakumov", "abalation", "abalienate", "abalienated", "abalienating", "abalienation", "abalone", "abalones", "Abama", "abamp", "abampere", "abamperes", "abamps", "Abana", "aband", "abandon", "abandonable", "abandoned", "abandonedly", "abandonee", "abandoner", "abandoners", "abandoning", "abandonment", "abandonments", "abandons", "abandum", "abanet", "abanga", "Abanic", "abannition", "Abantes", "abapical", "abaptiston", "abaptistum", "Abarambo", "Abarbarea", "Abaris", "abarthrosis", "abarticular", "abarticulation", "Abas", "abas", "abase", "abased", "abasedly", "abasedness", "abasement", "abasements", "abaser", "abasers", "abases", "Abasgi", "abash", "abashed", "abashedly", "abashedness", "abashes", "abashing", "abashless", "abashlessly", "abashment", "abashments", "abasia", "abasias", "abasic", "abasing", "abasio", "abask", "abassi", "Abassieh", "Abassin", "abastard", "abastardize", "abastral", "abatable", "abatage", "Abate", "abate", "abated", "abatement", "abatements", "abater", "abaters", "abates", "abatic", "abating", "abatis", "abatised", "abatises", "abatjour", "abatjours", "abaton", "abator", "abators", "ABATS", "abattage", "abattis", "abattised", "abattises", "abattoir", "abattoirs", "abattu", "abattue", "Abatua", "abature", "abaue", "abave", "abaxial", "abaxile", "abay", "abayah", "abaze", "abb", "Abba", "abba"};
  
  MemcachedStore store(false, "./cluster_settings", NULL, NULL);
  store.new_view(orig_servers, {});
  printf("*** Writing to original set of servers\n");
  for (int ii = 0; ii < test_words.size(); ii++)
  {
    store.set_data(unique_name, test_words[ii], test_words[ii], 0, 100);
  }
  printf("*** Beginning scale-up\n");
  store.new_view(orig_servers, new_servers);
  printf("*** Reading during scale-up\n");
  for (int ii = 0; ii < test_words.size(); ii++)
  {
    std::string out;
    uint64_t cas = 0;
    store.get_data(unique_name, test_words[ii], out, cas);
    if (out != test_words[ii])
    {
      printf("!!! Mid-scale-up: could not successfully read %s\n", test_words[ii].c_str());
      return false;
    }
  }

  printf("*** Writing during scale-up\n");
  for (int ii = 0; ii < test_words_2.size(); ii++)
  {
    store.set_data(unique_name, test_words_2[ii], test_words_2[ii], 0, 100);
  }
  printf("*** Finishing scale-up\n");
  store.new_view(new_servers, {});
  printf("*** Reading from final set of servers\n");
  for (int ii = 0; ii < test_words_2.size(); ii++)
  {
    std::string out;
    uint64_t cas = 0;
    store.get_data(unique_name, test_words_2[ii], out, cas);
    if (out != test_words_2[ii])
    {
      printf("Post-scale-up: could not successfully read %s\n", test_words_2[ii].c_str());
      return false;
    }
  }
  return true;
}

int main()
{
  std::vector<std::string> base = {"127.0.0.1:11211", "127.0.0.1:11212", "127.0.0.1:11213"};
  std::vector<std::string> scale_up_suffix = {"127.0.0.1:11211", "127.0.0.1:11212", "127.0.0.1:11213", "127.0.0.1:11214"};
  std::vector<std::string> scale_up_prefix = {"127.0.0.1:11214", "127.0.0.1:11211", "127.0.0.1:11212", "127.0.0.1:11213"};

  std::vector<std::string> scale_down_suffix = {"127.0.0.1:11211", "127.0.0.1:11212"};
  std::vector<std::string> scale_down_prefix = {"127.0.0.1:11212", "127.0.0.1:11213"};

  std::vector<std::string> entirely_different = {"127.0.0.1:11214", "127.0.0.1:11215"};

  bool scale_up_suffix_passed = test_scaling("scale_up_suffix", base, scale_up_suffix);
  bool scale_up_prefix_passed = test_scaling("scale_up_prefix", base, scale_up_prefix);
  bool scale_down_suffix_passed = test_scaling("scale_down_suffix", base, scale_down_suffix);
  bool scale_down_prefix_passed = test_scaling("scale_down_prefix", base, scale_down_prefix);
  bool different_passed = test_scaling("different", base, entirely_different);

  printf("\n\n\n");
  printf("Test of scaling up (new servers at end) %s\n", scale_up_suffix_passed ? "passed" : "failed");
  printf("Test of scaling up (new servers at start) %s\n", scale_up_prefix_passed ? "passed" : "failed");
  printf("Test of scaling down (servers removed from end) %s\n", scale_down_suffix_passed ? "passed" : "failed");
  printf("Test of scaling down (servers removed from start) %s\n", scale_down_prefix_passed ? "passed" : "failed");
  printf("Test of scaling down (different set of servers) %s\n", different_passed ? "passed" : "failed");

  return 0;
}
