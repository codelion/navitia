/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!
  
LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
   
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
   
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
  
Stay tuned using
twitter @navitia 
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_autocomplete

#include "autocomplete/autocomplete.h"
#include "autocomplete/autocomplete_api.h"
#include "type/data.h"
#include <boost/test/unit_test.hpp>
#include <vector>
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <fstream>
#include<map>
#include<unordered_map>
#include "type/type.h"
#include <boost/regex.hpp>
#include "georef/adminref.h"
#include "georef/georef.h"
#include "routing/raptor.h"
#include "ed/build_helper.h"

namespace pt = boost::posix_time;
using namespace navitia::autocomplete;
using namespace navitia::georef;

BOOST_AUTO_TEST_CASE(parse_find_with_synonym_and_synonyms_test){
    int nbmax = 10;
    std::vector<std::string> admins;
    std::string admin_uri = "";

    autocomplete_map synonyms;
    std::set<std::string> ghostwords;
    synonyms["hotel de ville"]="mairie";
    synonyms["cc"]="centre commercial";
    synonyms["ld"]="Lieu-Dit";

    synonyms["st"]="saint";
    synonyms["ste"]="sainte";
    synonyms["cc"]="centre commercial";
    synonyms["chu"]="hopital";
    synonyms["chr"]="hopital";
    synonyms["bvd"]="boulevard";
    synonyms["bld"]="boulevard";
    synonyms["bd"]="boulevard";

    Autocomplete<unsigned int> ac;
    ac.add_string("hotel de ville paris", 0, ghostwords, synonyms);
    ac.add_string("r jean jaures", 1, ghostwords, synonyms);
    ac.add_string("rue jeanne d'arc", 2, ghostwords, synonyms);
    ac.add_string("avenue jean jaures", 3, ghostwords, synonyms);
    ac.add_string("boulevard poniatowski", 4, ghostwords, synonyms);
    ac.add_string("pente de Bray", 5, ghostwords, synonyms);
    ac.add_string("Centre Commercial Caluire 2", 6, ghostwords, synonyms);
    ac.add_string("Rue René", 7, ghostwords, synonyms);
    ac.build();

    //Dans le dictionnaire : "hotel de ville paris" -> "mairie paris"
    //Recherche : "mai paris" -> "mai paris"
    // distance = 3 / word_weight = 0*5 = 0
    // Qualité = 100 - (3 + 0) = 97
    auto res = ac.find_complete("mai paris", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res.at(0).quality, 100);

    //Dans le dictionnaire : "hotel de ville paris" -> "mairie paris"
    //Recherche : "hotel de ville par" -> "mairie par"
    // distance = 3 / word_weight = 0*5 = 0
    // Qualité = 100 - (2 + 0) = 98
    auto res1 = ac.find_complete("hotel de ville par", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.at(0).quality, 100);

    //Dans le dictionnaire : "Centre Commercial Caluire 2" -> "centre commercial Caluire 2"
    //Recherche : "c c ca 2" -> "centre commercial Ca"
    // distance = 5 / word_weight = 0*5 = 0
    // Qualité = 100 - (5 + 0) = 95

    auto res2 = ac.find_complete("c c ca 2",nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res2.size(), 1);
    BOOST_CHECK_EQUAL(res2.at(0).quality, 100);

    auto res3 = ac.find_complete("cc ca 2", nbmax,[](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res3.size(), 1);
    BOOST_CHECK_EQUAL(res3.at(0).quality, 100);

    auto res4 = ac.find_complete("rue rene", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res4.size(), 1);
    BOOST_CHECK_EQUAL(res4.at(0).quality, 100);

    auto res5 = ac.find_complete("rue rené", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res5.size(), 1);
    BOOST_CHECK_EQUAL(res5.at(0).quality, 100);
}

BOOST_AUTO_TEST_CASE(regex_tests){
    boost::regex re("\\<c c\\>");
    BOOST_CHECK(boost::regex_search("c c", re));
    BOOST_CHECK(boost::regex_search("bonjour c c", re));
    BOOST_CHECK(boost::regex_search("c c au revoir", re));
    BOOST_CHECK(!boost::regex_search("ac c", re));
    BOOST_CHECK(!boost::regex_search("c ca", re));
    BOOST_CHECK(boost::regex_search("c c'a", re));
    BOOST_CHECK(boost::regex_search("a c c a", re));
}

BOOST_AUTO_TEST_CASE(regex_strip_accents_tests){
    std::map<char, char> map_accents;

    Autocomplete<unsigned int> ac;
    std::string test("républiquê");
    BOOST_CHECK_EQUAL(ac.strip_accents(test) ,"republique");
    test = "ÂâÄäçÇÉéÈèÊêËëÖöÔôÜüÎîÏïæœ";
    BOOST_CHECK_EQUAL(ac.strip_accents(test) ,"aaaacceeeeeeeeoooouuiiiiaeoe");
}


BOOST_AUTO_TEST_CASE(regex_replace_tests){
    boost::regex re("\\<c c\\>");
    BOOST_CHECK_EQUAL( boost::regex_replace(std::string("c c"), re , "centre commercial"), "centre commercial");
    BOOST_CHECK_EQUAL( boost::regex_replace(std::string("cc c"), re , "centre commercial"), "cc c");
    BOOST_CHECK_EQUAL( boost::regex_replace(std::string("c cv"), re , "centre commercial"), "c cv");
    BOOST_CHECK_EQUAL( boost::regex_replace(std::string("bonjour c c"), re , "centre commercial"), "bonjour centre commercial");
    BOOST_CHECK_EQUAL( boost::regex_replace(std::string("c c revoir"), re , "centre commercial"), "centre commercial revoir");
    BOOST_CHECK_EQUAL( boost::regex_replace(std::string("bonjour c c revoir"), re , "centre commercial"), "bonjour centre commercial revoir");
}

BOOST_AUTO_TEST_CASE(regex_toknize_with_synonyms_tests){

    autocomplete_map synonyms;
    synonyms["hotel de ville"]="mairie";
    synonyms["cc"]="centre commercial";
    synonyms["ld"]="Lieu-Dit";
    synonyms["st"]="saint";

    std::set<std::string> ghostwords;

    Autocomplete<unsigned int> ac;
    std::set<std::string> vec;

    //synonyme : "cc" = "centre commercial" / synonym : de = ""
    //"cc Carré de Soie" -> "centre commercial carré de soie"
    vec = ac.tokenize("cc Carré de Soie", ghostwords, synonyms);
    BOOST_CHECK(vec.find("carre") != vec.end());
    BOOST_CHECK(vec.find("centre") != vec.end());
    BOOST_CHECK(vec.find("commercial") != vec.end());
    BOOST_CHECK(vec.find("soie") != vec.end());

    vec.clear();
    //synonyme : "cc"= "centre commercial" / synonym : de = ""
    //"c Carré de Soie" -> "centre commercial carré de soie"
    vec = ac.tokenize("cc Carré de Soie", ghostwords, synonyms);
    BOOST_CHECK(vec.find("carre") != vec.end());
    BOOST_CHECK(vec.find("centre") != vec.end());
    BOOST_CHECK(vec.find("commercial") != vec.end());
    BOOST_CHECK(vec.find("soie") != vec.end());
}

BOOST_AUTO_TEST_CASE(regex_toknize_without_synonyms_tests){

    Autocomplete<unsigned int> ac;
    std::set<std::string> vec;
    std::set<std::string> ghostwords;

    //"cc Carré de Soie" -> "cc carre de soie"
    vec = ac.tokenize("cc Carré de Soie", ghostwords);
    BOOST_CHECK(vec.find("cc") != vec.end());
    BOOST_CHECK(vec.find("carre") != vec.end());
    BOOST_CHECK(vec.find("de") != vec.end());
    BOOST_CHECK(vec.find("soie") != vec.end());
}

BOOST_AUTO_TEST_CASE(regex_toknize_without_tests){

    autocomplete_map synonyms;
    synonyms["hotel de ville"]="mairie";
    synonyms["cc"]="centre commercial";
    synonyms["ld"]="Lieu-Dit";
    synonyms["st"]="saint";

    std::set<std::string> ghostwords;

    Autocomplete<unsigned int> ac;
    std::set<std::string> vec;

    //synonyme : "cc" = "centre commercial"
    //"cc Carré de Soie" -> "cc centre commercial carré de soie"
    vec = ac.tokenize("cc Carré de Soie", ghostwords, synonyms);
    BOOST_CHECK(vec.find("cc") != vec.end());
    BOOST_CHECK(vec.find("carre") != vec.end());
    BOOST_CHECK(vec.find("centre") != vec.end());
    BOOST_CHECK(vec.find("commercial") != vec.end());
    BOOST_CHECK(vec.find("de") != vec.end());
    BOOST_CHECK(vec.find("soie") != vec.end());    
}

BOOST_AUTO_TEST_CASE(regex_address_type_tests){

    autocomplete_map synonyms;
    synonyms["hotel de ville"]="mairie";
    synonyms["cc"]="centre commercial";
    synonyms["ld"]="Lieu-Dit";
    synonyms["av"]="avenue";
    synonyms["r"]="rue";
    synonyms["bvd"]="boulevard";
    synonyms["bld"]="boulevard";
    synonyms["bd"]="boulevard";
    //AddressType = {"rue", "avenue", "place", "boulevard","chemin", "impasse"}
    std::set<std::string> ghostwords;

    Autocomplete<unsigned int> ac;
    bool addtype = ac.is_address_type("r", ghostwords, synonyms);
    BOOST_CHECK_EQUAL(addtype, true);
}

BOOST_AUTO_TEST_CASE(regex_synonyme_gare_sncf_tests){

    autocomplete_map synonyms;
    synonyms["gare sncf"]="gare";
    synonyms["st"]="saint";
    synonyms["bvd"]="boulevard";
    synonyms["bld"]="boulevard";
    synonyms["bd"]="boulevard";
    std::set<std::string> ghostwords;

    Autocomplete<unsigned int> ac;
    std::set<std::string> vec;

    //synonyme : "gare sncf" = "gare"
    //"gare sncf" -> "gare sncf"
    vec = ac.tokenize("gare sncf", ghostwords, synonyms);
    BOOST_CHECK_EQUAL(vec.size(),2);
    vec.clear();

    //synonyme : "gare sncf" = "gare"
    //"gare snc" -> "gare sncf snc"
    vec = ac.tokenize("gare snc", ghostwords, synonyms);
    BOOST_CHECK_EQUAL(vec.size(),3);
    vec.clear();

    //synonyme : "gare sncf" = "gare"
    //"gare sn  nantes" -> "gare sncf sn nantes"
    vec = ac.tokenize("gare sn  nantes", ghostwords, synonyms);
    BOOST_CHECK(vec.find("gare") != vec.end());
    BOOST_CHECK(vec.find("nantes") != vec.end());
    BOOST_CHECK(vec.find("sncf") != vec.end());
    BOOST_CHECK(vec.find("sn") != vec.end());
    BOOST_CHECK_EQUAL(vec.size(),4);
    vec.clear();

    //synonyme : "gare sncf" = "gare"
    //"gare s nantes" -> "gare s sncf nantes"
    vec = ac.tokenize("gare  s  nantes", ghostwords, synonyms);
    BOOST_CHECK(vec.find("gare") != vec.end());
    BOOST_CHECK(vec.find("nantes") != vec.end());
    BOOST_CHECK(vec.find("sncf") != vec.end());
    BOOST_CHECK(vec.find("s") != vec.end());
    BOOST_CHECK_EQUAL(vec.size(),4);
    vec.clear();
}

BOOST_AUTO_TEST_CASE(parse_find_with_name_in_vector_test){
    autocomplete_map synonyms;
    std::set<std::string> ghostwords;
    std::set<std::string> vec;
    std::string admin_uri = "";

    Autocomplete<unsigned int> ac;
    ac.add_string("rue jean jaures", 0, ghostwords, synonyms);
    ac.add_string("place jean jaures", 1, ghostwords, synonyms);
    ac.add_string("rue jeanne d'arc", 2, ghostwords, synonyms);
    ac.add_string("avenue jean jaures", 3, ghostwords, synonyms);
    ac.add_string("boulevard poniatowski", 4, ghostwords, synonyms);
    ac.add_string("pente de Bray", 5, ghostwords, synonyms);
    ac.build();

    vec.insert("rue");
    vec.insert("jean");
    vec.insert("jaures");
    auto res = ac.find(vec);
    std::vector<int> expected = {0};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    vec.clear();
    vec.insert("jaures");
    res = ac.find(vec);
    expected = {0, 1, 3};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    vec.clear();
    vec.insert("avenue");
    res = ac.find(vec);
    expected = {3};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    vec.clear();
    vec.insert("av");
    res = ac.find(vec);
    expected = {3};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    vec.clear();
    vec.insert("r");
    vec.insert("jean");
    res = ac.find(vec);
    expected = {0, 2};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    vec.clear();
    vec.insert("jean");
    vec.insert("r");
    res = ac.find(vec);
    expected = {0, 2};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    vec.clear();
    vec.insert("ponia");
    res = ac.find(vec);
    expected = {4};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    vec.clear();
    vec.insert("ru");
    vec.insert("je");
    vec.insert("jau");
    res = ac.find(vec);
    expected = {0};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());    
}

/*
    > Le fonctionnement partiel :> On prends tous les autocomplete s'il y au moins un match
      et trie la liste des Autocomplete par la qualité.
    > La qualité est calculé avec le nombre des mots dans la recherche et le nombre des matchs dans Autocomplete.

    Exemple:
    vector[] = {0,1,2,3,4,5};
    Recherche : "rue jean";
    Vector      nb_found    Base Quality    Distance    Penalty Quality
        0           1       33              15-7 = 8    2       33-8-2 = 23
        1           2       50              13-7 = 6    2       50-6-2 = 42
        2           1       33              16-7 = 9    2       33-9-2 = 22
        5           2       66              13-7 = 6    2       66-6-2 = 58
    Result [] = {1,5,0,2}

    Recherche : "jean";
    Vector      nb_found    Quality
        0           1       100
        1           1       100
        2           1       100
        5           1       100

    > Le résultat est trié par qualité
    Result [] = {0,1,2,5}
*/

BOOST_AUTO_TEST_CASE(Faute_de_frappe_One){

        autocomplete_map synonyms;
        std::set<std::string> ghostwords;
        int word_weight = 5;
        int nbmax = 10;

        Autocomplete<unsigned int> ac;

        ac.add_string("gare Château", 0, ghostwords, synonyms);
        ac.add_string("gare bateau", 1, ghostwords, synonyms);
        ac.add_string("gare de taureau", 2, ghostwords, synonyms);
        ac.add_string("gare tauro", 3, ghostwords, synonyms);
        ac.add_string("gare gateau", 4, ghostwords, synonyms);

        ac.build();

        auto res = ac.find_partial_with_pattern("batau", word_weight, nbmax, [](int){return true;}, ghostwords);
        BOOST_REQUIRE_EQUAL(res.size(), 1);
        BOOST_CHECK_EQUAL(res.at(0).idx, 1);
        BOOST_CHECK_EQUAL(res.at(0).quality, 90);

        auto res1 = ac.find_partial_with_pattern("gare patea", word_weight, nbmax, [](int){return true;}, ghostwords);
        BOOST_REQUIRE_EQUAL(res1.size(), 3);
        std::initializer_list<unsigned> best_res = {1, 4}; // they are equivalent
        BOOST_CHECK(in(res1.at(0).idx, best_res));
        BOOST_CHECK(in(res1.at(1).idx, best_res));
        BOOST_CHECK_NE(res1.at(0).idx, res1.at(1).idx);
        BOOST_CHECK_EQUAL(res1.at(2).idx, 0);
        BOOST_CHECK_EQUAL(res1.at(0).quality, 94);
    }

/*
    > Le fonctionnement normal :> On prends que les autocomplete si tous les mots dans la recherche existent
      et trie la liste des Autocomplete par la qualité.
    > La qualité est calculé avec le nombre des mots dans la recherche et le nombre des mots dans autocomplete remonté.
sort
    Exemple:
    vector[] = {0,1,2,3,4,5,6,7,8};
    Recherche : "rue jean";
    Vector      nb_found    AC_wordcount    Base Quality  Distance    Penalty Quality
        0           2           4               50        13-7=6        2       50-6-2 = 42
        2           2           5               40        24-7=17       2       40-17-2= 21
        6           2           3               66        13-7=6        2       66-6-2 = 58
        7           2           3               66        10-7=3        2       66-3-2 = 61

    > Le résultat est trié par qualité.
    Result [] = {6,7,0,2}
*/

BOOST_AUTO_TEST_CASE(autocomplete_find_quality_test){

    autocomplete_map synonyms;
    std::set<std::string> ghostwords;
    std::vector<std::string> admins;
    std::string admin_uri = "";
    int nbmax = 10;

    Autocomplete<unsigned int> ac;
    ac.add_string("rue jeanne d'arc", 0, ghostwords, synonyms);
    ac.add_string("place jean jaures", 1, ghostwords, synonyms);
    ac.add_string("rue jean paul gaultier paris", 2, ghostwords, synonyms);
    ac.add_string("avenue jean jaures", 3, ghostwords, synonyms);
    ac.add_string("boulevard poniatowski", 4, ghostwords, synonyms);
    ac.add_string("pente de Bray", 5, ghostwords, synonyms);
    ac.add_string("rue jean jaures", 6, ghostwords, synonyms);
    ac.add_string("rue jean zay ", 7, ghostwords, synonyms);
    ac.add_string("place jean paul gaultier ", 8, ghostwords, synonyms);

    ac.build();

    auto res = ac.find_complete("rue jean", nbmax,[](int){return true;}, ghostwords);
    std::vector<int> expected = {6,7,0,2};
    BOOST_REQUIRE_EQUAL(res.size(), 4);
    BOOST_CHECK_EQUAL(res.at(0).quality, 100);
    BOOST_CHECK_EQUAL(res.at(1).quality, 100);
    BOOST_CHECK_EQUAL(res.at(2).quality, 100);
    BOOST_CHECK_EQUAL(res.at(3).quality, 100);
    BOOST_CHECK_EQUAL(res.at(0).idx, 2);
    BOOST_CHECK_EQUAL(res.at(1).idx, 7);
    BOOST_CHECK_EQUAL(res.at(2).idx, 0);
    BOOST_CHECK_EQUAL(res.at(3).idx, 6);
}

///Test pour verifier que - entres les deux mots est ignoré.
BOOST_AUTO_TEST_CASE(autocomplete_add_string_with_Line){

    autocomplete_map synonyms;
    std::set<std::string> ghostwords;
    std::vector<std::string> admins;
    std::string admin_uri = "";
    int nbmax = 10;

    Autocomplete<unsigned int> ac;
    ac.add_string("rue jeanne d'arc", 0, ghostwords, synonyms);
    ac.add_string("place jean jaures", 1, ghostwords, synonyms);
    ac.add_string("avenue jean jaures", 3, ghostwords, synonyms);
    ac.add_string("rue jean-jaures", 6, ghostwords, synonyms);
    ac.add_string("rue jean zay ", 7, ghostwords, synonyms);

    ac.build();

    auto res = ac.find_complete("jean-jau", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res.size(), 3);
    BOOST_CHECK_EQUAL(res.at(0).idx, 1);
    BOOST_CHECK_EQUAL(res.at(1).idx, 3);
    BOOST_CHECK_EQUAL(res.at(2).idx, 6);
}

BOOST_AUTO_TEST_CASE(autocompletesynonym_and_weight_test){

        autocomplete_map synonyms;
        std::set<std::string> ghostwords;
        std::vector<std::string> admins;
        std::string admin_uri;
        int nbmax = 10;        
        synonyms["st"]="saint";
        synonyms["ste"]="sainte";
        synonyms["cc"]="centre commercial";
        synonyms["chu"]="hopital";
        synonyms["chr"]="hopital";
        synonyms["bvd"]="boulevard";
        synonyms["bld"]="boulevard";
        synonyms["bd"]="boulevard";

        Autocomplete<unsigned int> ac;
        ac.add_string("rue jeanne d'arc", 0, ghostwords, synonyms);
        ac.add_string("place jean jaures", 1, ghostwords, synonyms);
        ac.add_string("rue jean paul gaultier paris", 2, ghostwords, synonyms);
        ac.add_string("avenue jean jaures", 3, ghostwords, synonyms);
        ac.add_string("boulevard poniatowski", 4, ghostwords, synonyms);
        ac.add_string("pente de Bray", 5, ghostwords, synonyms);
        ac.add_string("rue jean jaures", 6, ghostwords, synonyms);
        ac.add_string("rue jean zay ", 7, ghostwords, synonyms);
        ac.add_string("place jean paul gaultier ", 8, ghostwords, synonyms);
        ac.add_string("hopital paul gaultier", 8, ghostwords, synonyms);

        ac.build();

        auto res = ac.find_complete("rue jean", nbmax, [](int){return true;}, ghostwords);
        BOOST_REQUIRE_EQUAL(res.size(), 4);
        BOOST_CHECK_EQUAL(res.at(0).quality, 100);

        auto res1 = ac.find_complete("r jean", nbmax, [](int){return true;}, ghostwords);
        BOOST_REQUIRE_EQUAL(res1.size(), 4);

        BOOST_CHECK_EQUAL(res1.at(0).quality, 100);

        auto res2 = ac.find_complete("av jean", nbmax, [](int){return true;}, ghostwords);
        BOOST_REQUIRE_EQUAL(res2.size(), 1);
        //rue jean zay
        // distance = 6 / word_weight = 1*5 = 5
        // Qualité = 100 - (6 + 5) = 89
        BOOST_CHECK_EQUAL(res2.at(0).quality, 100);

        auto res3 = ac.find_complete("av jean", nbmax,[](int){return true;}, ghostwords);
        BOOST_REQUIRE_EQUAL(res3.size(), 1);
        //rue jean zay
        // distance = 6 / word_weight = 1*10 = 10
        // Qualité = 100 - (6 + 10) = 84
        BOOST_CHECK_EQUAL(res3.at(0).quality, 100);

        auto res4 = ac.find_complete("chu gau", nbmax, [](int){return true;}, ghostwords);
        BOOST_REQUIRE_EQUAL(res4.size(), 1);
        //hopital paul gaultier
        // distance = 9 / word_weight = 1*10 = 10
        // Qualité = 100 - (9 + 10) = 81
        BOOST_CHECK_EQUAL(res4.at(0).quality, 100);
    }

BOOST_AUTO_TEST_CASE(autocomplete_duplicate_words_and_weight_test){

    autocomplete_map synonyms;
    std::set<std::string> ghostwords;
    std::vector<std::string> admins;
    std::string admin_uri;
    int nbmax = 10;

    synonyms["st"]="saint";
    synonyms["ste"]="sainte";
    synonyms["cc"]="centre commercial";
    synonyms["chu"]="hopital";
    synonyms["chr"]="hopital";
    synonyms["bvd"]="boulevard";
    synonyms["bld"]="boulevard";
    synonyms["bd"]="boulevard";

    Autocomplete<unsigned int> ac;
    ac.add_string("gare de Tours Tours", 0, ghostwords, synonyms);
    ac.add_string("Garennes-sur-Eure", 1, ghostwords, synonyms);
    ac.add_string("gare d'Orléans Orléans", 2, ghostwords, synonyms);
    ac.add_string("gare de Bourges Bourges", 3, ghostwords, synonyms);
    ac.add_string("Gare SNCF Blois", 4, ghostwords, synonyms);
    ac.add_string("gare de Dreux Dreux", 5, ghostwords, synonyms);
    ac.add_string("gare de Lucé Lucé", 6, ghostwords, synonyms);
    ac.add_string("gare de Vierzon Vierzon", 7, ghostwords, synonyms);
    ac.add_string("Les Sorinières, Les Sorinières", 8, ghostwords, synonyms);
    ac.add_string("Les Sorinières 44840", 9, ghostwords, synonyms);
    ac.add_string("Rouet, Les Sorinières", 10, ghostwords, synonyms);
    ac.add_string("Ecoles, Les Sorinières", 11, ghostwords, synonyms);
    ac.add_string("Prières, Les Sorinières", 12, ghostwords, synonyms);
    ac.add_string("Calvaire, Les Sorinières", 13, ghostwords, synonyms);
    ac.add_string("Corberie, Les Sorinières", 14, ghostwords, synonyms);
    ac.add_string("Le Pérou, Les Sorinières", 15, ghostwords, synonyms);
    ac.add_string("Les Sorinières, Villebernier", 16, ghostwords, synonyms);
    ac.add_string("Les Faulx, Les Sorinières", 17, ghostwords, synonyms);
    ac.add_string("cimetière, Les Sorinières", 18, ghostwords, synonyms);

    ac.build();

    auto res = ac.find_complete("gare", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res.size(), 8);
    BOOST_CHECK_EQUAL(res.at(0).quality, 100);
    BOOST_CHECK_EQUAL(res.at(0).idx, 7);
    BOOST_CHECK_EQUAL(res.at(1).idx, 4);
    BOOST_CHECK_EQUAL(res.at(2).idx, 1);
    BOOST_CHECK_EQUAL(res.at(3).idx, 3);
    BOOST_CHECK_EQUAL(res.at(4).idx, 5);
    BOOST_CHECK_EQUAL(res.at(5).idx, 0);
    BOOST_CHECK_EQUAL(res.at(6).idx, 2);
    BOOST_CHECK_EQUAL(res.at(7).idx, 6);
    BOOST_CHECK_EQUAL(res.at(7).quality, 100);


    auto res1 = ac.find_complete("gare tours", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.at(0).quality, 100);

    auto res2 = ac.find_complete("gare tours tours", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_CHECK_EQUAL(res1.at(0).quality, 100);

    auto res3 = ac.find_complete("les Sorinières", nbmax,[](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res3.size(), 10);
    BOOST_CHECK_EQUAL(res3.at(0).quality, 100);
}

/*
1. We have 1 administrative_region and 11  stop_area
2. All the stop_areas are attached to the same administrative_region.
3. Call with "quimer" and count = 10
4. In the result the administrative_region is the first one
5. All the stop_areas with same quality are sorted by name (case insensitive).
6. There 10 elements in the result.
7. We don't have penalty on "admin weight" and "stoparea weight".
*/
BOOST_AUTO_TEST_CASE(autocomplete_functional_test_admin_and_SA_test) {
    std::vector<std::string> admins;
    std::vector<navitia::type::Type_e> type_filter;
    ed::builder b("20140614");
    b.sa("IUT", 0, 0);
    b.sa("Gare", 0, 0);
    b.sa("Resistance", 0, 0);
    b.sa("Becharles", 0, 0);
    b.sa("Yoyo", 0, 0);
    b.sa("Luther King", 0, 0);
    b.sa("Napoleon III", 0, 0);
    b.sa("MPT kerfeunteun", 0, 0);
    b.sa("Marcel Paul", 0, 0);
    b.sa("chaptal", 0, 0);
    b.sa("Zebre", 0, 0);

    b.data->pt_data->index();
    Admin* ad = new Admin;
    ad->name = "Quimper";
    ad->uri = "Quimper";
    ad->level = 8;
    ad->postal_codes.push_back("29000");
    ad->idx = 0;
    b.data->geo_ref->admins.push_back(ad);
    b.manage_admin();
    b.build_autocomplete();

    type_filter.push_back(navitia::type::Type_e::StopArea);
    type_filter.push_back(navitia::type::Type_e::Admin);
    pbnavitia::Response resp = navitia::autocomplete::autocomplete("quimper", type_filter , 1, 10, admins, 0,
                                                                   *(b.data), boost::gregorian::not_a_date_time);

    BOOST_REQUIRE_EQUAL(resp.places_size(), 10);
    BOOST_CHECK_EQUAL(resp.places(0).embedded_type() , pbnavitia::ADMINISTRATIVE_REGION);
    BOOST_CHECK_EQUAL(resp.places(0).quality(), 90);
    BOOST_CHECK_EQUAL(resp.places(1).quality(), 90);
    BOOST_CHECK_EQUAL(resp.places(7).quality(), 90);
    BOOST_CHECK_EQUAL(resp.places(8).quality(), 80);
    BOOST_CHECK_EQUAL(resp.places(0).uri(), "Quimper");
    BOOST_CHECK_EQUAL(resp.places(1).uri(), "Becharles");
    BOOST_CHECK_EQUAL(resp.places(7).uri(), "Zebre");
    BOOST_CHECK_EQUAL(resp.places(8).uri(), "Luther King");
    BOOST_CHECK_EQUAL(resp.places(9).uri(), "Marcel Paul");

    resp = navitia::autocomplete::autocomplete("qui", type_filter , 1, 10, admins, 0, *(b.data),
                                               boost::gregorian::not_a_date_time);
    BOOST_REQUIRE_EQUAL(resp.places_size(), 10);
    BOOST_CHECK_EQUAL(resp.places(0).embedded_type() , pbnavitia::ADMINISTRATIVE_REGION);
    BOOST_CHECK_EQUAL(resp.places(0).uri(), "Quimper");
    BOOST_CHECK_EQUAL(resp.places(1).uri(), "Becharles");
    BOOST_CHECK_EQUAL(resp.places(9).uri(), "Marcel Paul");
}

/*
1. We have 1 administrative_region and 9  stop_area
2. All the stop_areas are attached to the same administrative_region.
3. Call with "quimer" and count = 5
4. In the result the administrative_region is absent
5. We don't have penalty on "admin weight" and "stoparea weight".
*/
BOOST_AUTO_TEST_CASE(autocomplete_functional_test_SA_test) {
    std::vector<std::string> admins;
    std::vector<navitia::type::Type_e> type_filter;
    ed::builder b("20140614");
    b.sa("IUT", 0, 0);
    b.sa("Gare SNCF", 0, 0);
    b.sa("Resistance", 0, 0);
    b.sa("Becharles", 0, 0);
    b.sa("Luther King", 0, 0);
    b.sa("Napoleon III", 0, 0);
    b.sa("MPT kerfeunteun", 0, 0);
    b.sa("Marcel Paul", 0, 0);
    b.sa("chaptal", 0, 0);
    b.sa("Tourbie", 0, 0);
    b.sa("Bourgogne", 0, 0);

    b.data->pt_data->index();
    Admin* ad = new Admin;
    ad->name = "Quimper";
    ad->uri = "Quimper";
    ad->level = 8;
    ad->postal_codes.push_back("29000");
    ad->idx = 0;
    b.data->geo_ref->admins.push_back(ad);
    b.manage_admin();
    b.build_autocomplete();

    type_filter.push_back(navitia::type::Type_e::StopArea);
    type_filter.push_back(navitia::type::Type_e::Admin);
    pbnavitia::Response resp = navitia::autocomplete::autocomplete("quimper", type_filter , 1, 5, admins, 0,
                                                                   *(b.data), boost::gregorian::not_a_date_time);

    BOOST_REQUIRE_EQUAL(resp.places_size(), 5);
    BOOST_CHECK_EQUAL(resp.places(0).embedded_type() , pbnavitia::ADMINISTRATIVE_REGION);
    BOOST_CHECK_EQUAL(resp.places(0).quality(), 90);
    BOOST_CHECK_EQUAL(resp.places(1).quality(), 90);
    BOOST_CHECK_EQUAL(resp.places(0).uri(), "Quimper");
    BOOST_CHECK_EQUAL(resp.places(4).uri(), "IUT");
}

/*
1. We have 1 administrative_region ,6 stop_area and 3 way
2. All these objects are attached to the same administrative_region.
3. Call with "quimer" and count = 10
4. In the result the administrative_region is the first one
5. All the stop_areas with same quality are sorted by name (case insensitive).
6. The addresses have different quality values due to diferent number of words.
7. There 10 elements in the result.
8. We don't have penalty on "admin weight" and "stoparea weight".
*/
BOOST_AUTO_TEST_CASE(autocomplete_functional_test_admin_SA_and_Address_test) {
    std::vector<std::string> admins;
    std::vector<navitia::type::Type_e> type_filter;
    ed::builder b("20140614");
    b.sa("IUT", 0, 0);
    b.sa("Gare", 0, 0);
    b.sa("Becharles", 0, 0);
    b.sa("Luther King", 0, 0);
    b.sa("Napoleon III", 0, 0);
    b.sa("MPT kerfeunteun", 0, 0);
    b.data->pt_data->index();
    Way w;
    w.idx = 0;
    w.name = "rue DU TREGOR";
    w.uri = w.name;
    b.data->geo_ref->add_way(w);
    w.name = "rue VIS";
    w.uri = w.name;
    w.idx = 1;
    b.data->geo_ref->add_way(w);
    w.idx = 2;
    w.name = "quai NEUF";
    w.uri = w.name;
    b.data->geo_ref->add_way(w);

    Admin* ad = new Admin;
    ad->name = "Quimper";
    ad->uri = "Quimper";
    ad->level = 8;
    ad->postal_codes.push_back("29000");
    ad->idx = 0;
    b.data->geo_ref->admins.push_back(ad);
    b.manage_admin();
    b.build_autocomplete();

    type_filter.push_back(navitia::type::Type_e::StopArea);
    type_filter.push_back(navitia::type::Type_e::Admin);
    pbnavitia::Response resp = navitia::autocomplete::autocomplete("quimper", type_filter , 1, 10, admins, 0,
                                                                   *(b.data), boost::gregorian::not_a_date_time);
    //Here we want only Admin and StopArea
    BOOST_REQUIRE_EQUAL(resp.places_size(), 7);
    type_filter.push_back(navitia::type::Type_e::Address);
    resp = navitia::autocomplete::autocomplete("quimper", type_filter , 1, 10, admins, 0, *(b.data),
                                               boost::gregorian::not_a_date_time);

    BOOST_REQUIRE_EQUAL(resp.places_size(), 10);
    BOOST_CHECK_EQUAL(resp.places(0).embedded_type() , pbnavitia::ADMINISTRATIVE_REGION);
    BOOST_CHECK_EQUAL(resp.places(0).quality(), 90);
    BOOST_CHECK_EQUAL(resp.places(1).quality(), 90);

    BOOST_CHECK_EQUAL(resp.places(0).uri(), "Quimper");
    BOOST_CHECK_EQUAL(resp.places(1).uri(), "Becharles");
    BOOST_CHECK_EQUAL(resp.places(7).name(), "quai NEUF (Quimper)");
    BOOST_CHECK_EQUAL(resp.places(8).name(), "rue VIS (Quimper)");
    BOOST_CHECK_EQUAL(resp.places(9).name(), "rue DU TREGOR (Quimper)");
}

/*
1. We have 1 Network (name="base_network"), 2 mode (name="Tram" et name="Metro")
2. 1 line for mode "Tram" and and 2 routes
*/
BOOST_AUTO_TEST_CASE(autocomplete_pt_object_Network_Mode_Line_Route_test) {
    std::vector<std::string> admins;
    std::vector<navitia::type::Type_e> type_filter;
    ed::builder b("201409011T1739");
    b.generate_dummy_basis();

    //Create a new line and affect it to mode "Tram"
    navitia::type::Line* line = new navitia::type::Line();
    line->idx = b.data->pt_data->lines.size();
    line->uri = "line 1";
    line->name = "line 1";
    line->commercial_mode = b.data->pt_data->commercial_modes[0];
    b.data->pt_data->lines.push_back(line);

    //Add two routes in the line
    navitia::type::Route* route = new navitia::type::Route();
    route->idx = b.data->pt_data->routes.size();
    route->name = line->name;
    route->uri = line->name + ":" + std::to_string(b.data->pt_data->routes.size());
    b.data->pt_data->routes.push_back(route);
    line->route_list.push_back(route);
    route->line = line;

    route = new navitia::type::Route();
    route->idx = b.data->pt_data->routes.size();
    route->name = line->name;
    route->uri = line->name + ":" + std::to_string(b.data->pt_data->routes.size());
    b.data->pt_data->routes.push_back(route);
    line->route_list.push_back(route);
    route->line = line;

    b.build_autocomplete();

    type_filter = {navitia::type::Type_e::Network, navitia::type::Type_e::CommercialMode,
                  navitia::type::Type_e::Line, navitia::type::Type_e::Route};
    pbnavitia::Response resp = navitia::autocomplete::autocomplete("base", type_filter , 1, 10, admins, 0,
                                                                   *(b.data), boost::gregorian::not_a_date_time);
    //The result contains only network
    BOOST_REQUIRE_EQUAL(resp.places_size(), 1);
    BOOST_CHECK_EQUAL(resp.places(0).embedded_type(), pbnavitia::NETWORK);

    // Call with "Tram" and &type[]=network&type[]=mode&type[]=line&type[]=route
    resp = navitia::autocomplete::autocomplete("Tram", type_filter , 1, 10, admins, 0, *(b.data),
                                               boost::gregorian::not_a_date_time);
    //In the result the first line is Mode and the second is line
    BOOST_REQUIRE_EQUAL(resp.places_size(), 4);
    BOOST_CHECK_EQUAL(resp.places(0).embedded_type(), pbnavitia::COMMERCIAL_MODE);
    BOOST_CHECK_EQUAL(resp.places(1).embedded_type(), pbnavitia::LINE);
    BOOST_CHECK_EQUAL(resp.places(2).embedded_type(), pbnavitia::ROUTE);
    BOOST_CHECK_EQUAL(resp.places(3).embedded_type(), pbnavitia::ROUTE);

    // Call with "line" and &type[]=network&type[]=mode&type[]=line&type[]=route
    resp = navitia::autocomplete::autocomplete("line", type_filter , 1, 10, admins, 0, *(b.data),
                                               boost::gregorian::not_a_date_time);
    //In the result the first line is line and the others are the routes
    BOOST_REQUIRE_EQUAL(resp.places_size(), 3);
    BOOST_CHECK_EQUAL(resp.places(0).embedded_type(), pbnavitia::LINE);
    BOOST_CHECK_EQUAL(resp.places(1).embedded_type(), pbnavitia::ROUTE);
    BOOST_CHECK_EQUAL(resp.places(2).embedded_type(), pbnavitia::ROUTE);
}

/*
1. We have 1 Network (name="base_network"), 2 mode (name="Tram" et name="Metro")
2. 1 line for mode Metro and and 2 routes
3. 4 stop_areas
*/
BOOST_AUTO_TEST_CASE(autocomplete_pt_object_Network_Mode_Line_Route_stop_area_test) {
    std::vector<std::string> admins;
    std::vector<navitia::type::Type_e> type_filter;
    ed::builder b("201409011T1739");
    b.generate_dummy_basis();

    //Create a new line and affect it to mode "Metro"
    navitia::type::Line* line = new navitia::type::Line();
    line->idx = b.data->pt_data->lines.size();
    line-> uri = "line 1";
    line-> name = "Chatelet - Vincennes";
    line->commercial_mode = b.data->pt_data->commercial_modes[1];
    b.data->pt_data->lines.push_back(line);

    //Add two routes in the line
    navitia::type::Route* route = new navitia::type::Route();
    route->idx = b.data->pt_data->routes.size();
    route->name = line->name;
    route->uri = line->name + ":" + std::to_string(b.data->pt_data->routes.size());
    b.data->pt_data->routes.push_back(route);
    line->route_list.push_back(route);
    route->line = line;

    route = new navitia::type::Route();
    route->idx = b.data->pt_data->routes.size();
    route->name = line->name;
    route->uri = line->name + ":" + std::to_string(b.data->pt_data->routes.size());
    b.data->pt_data->routes.push_back(route);
    line->route_list.push_back(route);
    route->line = line;

    b.sa("base", 0, 0);
    b.sa("chatelet", 0, 0);
    b.sa("Chateau", 0, 0);
    b.sa("Mettray", 0, 0);
    Admin* ad = new Admin;
    ad->name = "paris";
    ad->uri = "paris";
    ad->level = 8;
    ad->postal_codes.push_back("75000");
    ad->idx = 0;
    b.data->geo_ref->admins.push_back(ad);
    b.manage_admin();

    b.build_autocomplete();

    type_filter = {navitia::type::Type_e::Network, navitia::type::Type_e::CommercialMode,
                  navitia::type::Type_e::Line, navitia::type::Type_e::Route,
                  navitia::type::Type_e::StopArea};
    //Call with q=base
    pbnavitia::Response resp = navitia::autocomplete::autocomplete("base", type_filter , 1, 10, admins, 0,
                                                                   *(b.data), boost::gregorian::not_a_date_time);
    //The result contains network and stop_area
    BOOST_REQUIRE_EQUAL(resp.places_size(), 2);
    BOOST_CHECK_EQUAL(resp.places(0).embedded_type(), pbnavitia::NETWORK);
    BOOST_CHECK_EQUAL(resp.places(1).embedded_type(), pbnavitia::STOP_AREA);

    //Call with q=met
    resp = navitia::autocomplete::autocomplete("met", type_filter , 1, 10, admins, 0, *(b.data),
                                               boost::gregorian::not_a_date_time);
    //The result contains mode, stop_area, line
    BOOST_REQUIRE_EQUAL(resp.places_size(), 5);
    BOOST_CHECK_EQUAL(resp.places(0).embedded_type(), pbnavitia::COMMERCIAL_MODE);
    BOOST_CHECK_EQUAL(resp.places(1).embedded_type(), pbnavitia::STOP_AREA);
    BOOST_CHECK_EQUAL(resp.places(2).embedded_type(), pbnavitia::LINE);
    BOOST_CHECK_EQUAL(resp.places(3).embedded_type(), pbnavitia::ROUTE);
    BOOST_CHECK_EQUAL(resp.places(4).embedded_type(), pbnavitia::ROUTE);

    //Call with q=chat
    resp = navitia::autocomplete::autocomplete("chat", type_filter , 1, 10, admins, 0, *(b.data),
                                               boost::gregorian::not_a_date_time);
    //The result contains 2 stop_areas, one line and 2 routes
    BOOST_REQUIRE_EQUAL(resp.places_size(), 5);
    BOOST_CHECK_EQUAL(resp.places(0).embedded_type(), pbnavitia::STOP_AREA);
    BOOST_CHECK_EQUAL(resp.places(1).embedded_type(), pbnavitia::STOP_AREA);
    BOOST_CHECK_EQUAL(resp.places(2).embedded_type(), pbnavitia::LINE);
    BOOST_CHECK_EQUAL(resp.places(3).embedded_type(), pbnavitia::ROUTE);
    BOOST_CHECK_EQUAL(resp.places(4).embedded_type(), pbnavitia::ROUTE);
}

BOOST_AUTO_TEST_CASE(find_with_synonyms_mairie_de_vannes_test){
    int nbmax = 10;
    autocomplete_map synonyms;
    std::set<std::string> ghostwords;
    synonyms["hotel de ville"]="mairie";
    synonyms["cc"]="centre commercial";
    synonyms["gare sncf"]="gare";
    synonyms["bd"]="boulevard";
    synonyms["bld"]="boulevard";
    synonyms["bvd"]="boulevard";
    synonyms["chr"]="hopital";
    synonyms["chu"]="hopital";
    synonyms["ld"]="lieu-dit";
    synonyms["pt"]="pont";
    synonyms["rle"]="ruelle";
    synonyms["rte"]="route";
    synonyms["sq"]="square";
    synonyms["st"]="saint";
    synonyms["ste"]="sainte";
    synonyms["vla"]="villa";

    Autocomplete<unsigned int> ac;
    ac.add_string("mairie vannes", 0, ghostwords, synonyms);
    ac.add_string("place hotel de ville vannes", 1, ghostwords, synonyms);
    ac.add_string("rue DE L'HOTEL DIEU vannes", 2, ghostwords, synonyms);
    ac.add_string("place vannes", 3, ghostwords, synonyms);
    ac.add_string("Hôtel-Dieu vannes", 4, ghostwords, synonyms);
    ac.add_string("Hôtel de Région vannes", 5, ghostwords, synonyms);
    ac.add_string("Centre Commercial Caluire 2", 6, ghostwords, synonyms);
    ac.add_string("Rue René", 7, ghostwords, synonyms);
    ac.build();

    //Dans le dictionnaire : "mairie vannes" -> "mairie hotel de ville vannes"
    //search : "hotel vannes" -> "hotel vannes" no synonym is applied for search string
    //Found : mairie vannes et place hotel de ville vannes
    auto res = ac.find_complete("mairie vannes", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res.size(), 2);

    //Dans le dictionnaire : "mairie vannes" -> "mairie hotel de ville vannes"
    //search : "hotel vannes" -> "hotel vannes" no synonym is applied for search
    //Found : mairie vannes, place hotel de ville vannes, rue DE L'HOTEL DIEU vannes, Hôtel-Dieu vannes et Hôtel de Région vannes
    auto res1 = ac.find_complete("hotel vannes", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res1.size(), 5);
}

BOOST_AUTO_TEST_CASE(find_with_synonyms_gare_with_or_without_sncf_test){
    int nbmax = 10;
    autocomplete_map synonyms;
    std::set<std::string> ghostwords;
    synonyms["hotel de ville"]="mairie";
    synonyms["cc"]="centre commercial";
    synonyms["gare sncf"]="gare";
    synonyms["bd"]="boulevard";
    synonyms["bld"]="boulevard";
    synonyms["bvd"]="boulevard";
    synonyms["chr"]="hopital";
    synonyms["chu"]="hopital";
    synonyms["ld"]="lieu-dit";
    synonyms["pt"]="pont";
    synonyms["rle"]="ruelle";
    synonyms["rte"]="route";
    synonyms["sq"]="square";
    synonyms["st"]="saint";
    synonyms["ste"]="sainte";
    synonyms["vla"]="villa";

    Autocomplete<unsigned int> ac;
    ac.add_string("gare SNCF et routière Rennes", 0, ghostwords, synonyms);
    ac.add_string("Pré Garel Rennes", 1, ghostwords, synonyms);
    ac.add_string("Gare Sud Féval Rennes", 2, ghostwords, synonyms);
    ac.add_string("Parking gare SNCF et routière Rennes", 3, ghostwords, synonyms);
    ac.add_string("place DE LA GARE Rennes", 4, ghostwords, synonyms);
    ac.add_string("rue DU PRE GAREL Rennes", 5, ghostwords, synonyms);
    ac.add_string("Parking SNCF Rennes", 6, ghostwords, synonyms);
    ac.add_string("parc de la victoire Rennes", 7, ghostwords, synonyms);
    ac.build();

    //Dans le dictionnaire :
    //"gare SNCF et routière Rennes" -> "gare sncf et routiere rennes"
    //"Pré Garel Rennes" -> "Pre garel rennes"
    //"Gare Sud Féval Rennes" -> "gare sud feval rennes sncf"
    //"Parking gare SNCF et routière Rennes" -> "parking gare sncf et routiere rennes"
    //"place DE LA GARE Rennes" -> "place de la gare rennes sncf"
    //"rue DU PRE GAREL Rennes" -> "rue du pre garel rennes"
    //"Parking SNCF Rennes" -> "parking sncf rennes"

    //search : "Gare SNCF Rennes" -> "gare sncf rennes" no synonym is applied for search string
    //Found : "gare SNCF et routière Rennes", "Gare Sud Féval Rennes", "Parking gare SNCF et routière Rennes" et "place DE LA GARE Rennes"
    auto res = ac.find_complete("gare sncf rennes", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res.size(), 4);

    //search : "SNCF Rennes" -> "sncf rennes" no synonym is applied for search string
    //Found : "gare SNCF et routière Rennes", "Gare Sud Féval Rennes", "Parking gare SNCF et routière Rennes" ,"place DE LA GARE Rennes" et "Parking SNCF Rennes"
    auto res1 = ac.find_complete("sncf rennes", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res1.size(), 5);

    //search : "Gare Rennes" -> "gare rennes" no synonym is applied for search string
    //Found : "gare SNCF et routière Rennes", "Gare Sud Féval Rennes", "Parking gare SNCF et routière Rennes" ,"place DE LA GARE Rennes" et "Parking SNCF Rennes"
    auto res2 = ac.find_complete("gare rennes", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res2.size(), 6);
}

BOOST_AUTO_TEST_CASE(autocomplete_functional_test_SA_temp_test) {
    //Test to verify order of places in the result:
    //sort by type : Admin first and then stop_area
    //sort by score (except quality=100 at the top)
    //sort by quality
    //sort by name
    std::vector<std::string> admins;
    std::vector<navitia::type::Type_e> type_filter;
    ed::builder b("20140614");
    b.sa("Santec", 0, 0);
    b.sa("Ar Santé Les Fontaines Nantes", 0, 0);
    b.sa("Santenay-Haut Nantes", 0, 0);
    b.sa("Roye-Agri-Santerre Nantes", 0, 0);
    b.sa("Fontenay-le-Comte-Santé Nantes", 0, 0);
    b.sa("gare de Santes Nantes", 0, 0);
    b.sa("gare de Santenay-les-Bains Nantes", 0, 0);
    b.sa("gare de Santeuil-le-Perchay Nantes", 0, 0);
    b.sa("chaptal", 0, 0);
    b.sa("Tourbie", 0, 0);
    b.sa("Bourgogne", 0, 0);

    b.data->pt_data->index();
    Admin* ad = new Admin;
    ad->name = "Santec";
    ad->uri = "Santec";
    ad->level = 8;
    ad->postal_codes.push_back("29000");
    ad->idx = 0;
    b.data->geo_ref->admins.push_back(ad);
    b.manage_admin();
    b.build_autocomplete();
    b.data->geo_ref->fl_admin.word_quality_list.at(0).score = 50;
    b.data->pt_data->stop_area_autocomplete.word_quality_list.at(0).score = 10;
    b.data->pt_data->stop_area_autocomplete.word_quality_list.at(1).score = 7;
    b.data->pt_data->stop_area_autocomplete.word_quality_list.at(2).score = 35;
    b.data->pt_data->stop_area_autocomplete.word_quality_list.at(3).score = 35;
    b.data->pt_data->stop_area_autocomplete.word_quality_list.at(4).score = 75;
    b.data->pt_data->stop_area_autocomplete.word_quality_list.at(5).score = 70;
    b.data->pt_data->stop_area_autocomplete.word_quality_list.at(6).score = 45;
    b.data->pt_data->stop_area_autocomplete.word_quality_list.at(7).score = 50;
    b.data->pt_data->stop_area_autocomplete.word_quality_list.at(8).score = 5;
    b.data->pt_data->stop_area_autocomplete.word_quality_list.at(9).score = 5;
    b.data->pt_data->stop_area_autocomplete.word_quality_list.at(10).score = 70;


    type_filter.push_back(navitia::type::Type_e::StopArea);
    type_filter.push_back(navitia::type::Type_e::Admin);
    pbnavitia::Response resp = navitia::autocomplete::autocomplete("Sante", type_filter , 1, 15, admins, 0,
                                                                   *(b.data), boost::gregorian::not_a_date_time);

    BOOST_REQUIRE_EQUAL(resp.places_size(), 12);
    BOOST_CHECK_EQUAL(resp.places(0).embedded_type() , pbnavitia::ADMINISTRATIVE_REGION);
    BOOST_CHECK_EQUAL(resp.places(1).embedded_type() , pbnavitia::STOP_AREA);
    BOOST_CHECK_EQUAL(resp.places(0).quality(), 90);
    BOOST_CHECK_EQUAL(resp.places(1).quality(), 100);
    BOOST_CHECK_EQUAL(resp.places(0).uri(), "Santec"); //Admin
    BOOST_CHECK_EQUAL(resp.places(1).uri(), "Santec"); //score = 7 but quality = 100
    BOOST_CHECK_EQUAL(resp.places(1).quality(), 100);
    BOOST_CHECK_EQUAL(resp.places(2).uri(), "Fontenay-le-Comte-Santé Nantes"); //score = 75
    BOOST_CHECK_EQUAL(resp.places(3).uri(), "Bourgogne"); //score = 70
    BOOST_CHECK_EQUAL(resp.places(3).quality(), 90);
    BOOST_CHECK_EQUAL(resp.places(4).uri(), "gare de Santes Nantes"); //score = 70
    BOOST_CHECK_EQUAL(resp.places(4).quality(), 60);
    BOOST_CHECK_EQUAL(resp.places(5).uri(), "gare de Santeuil-le-Perchay Nantes"); //score = 50
    BOOST_CHECK_EQUAL(resp.places(5).quality(), 40);
    BOOST_CHECK_EQUAL(resp.places(6).uri(), "gare de Santenay-les-Bains Nantes"); //score = 45
    BOOST_CHECK_EQUAL(resp.places(6).quality(), 40);
    BOOST_CHECK_EQUAL(resp.places(7).uri(), "Santenay-Haut Nantes"); //score = 35
    BOOST_CHECK_EQUAL(resp.places(7).quality(), 70);
    BOOST_CHECK_EQUAL(resp.places(8).uri(), "Roye-Agri-Santerre Nantes"); //score = 35
    BOOST_CHECK_EQUAL(resp.places(8).quality(), 60);
    BOOST_CHECK_EQUAL(resp.places(9).uri(), "Ar Santé Les Fontaines Nantes"); //score = 7
    BOOST_CHECK_EQUAL(resp.places(9).quality(), 50);
    BOOST_CHECK_EQUAL(resp.places(10).uri(), "chaptal"); // score = 5
    BOOST_CHECK_EQUAL(resp.places(10).quality(), 90);
    BOOST_CHECK_EQUAL(resp.places(11).uri(), "Tourbie"); //score = 5
    BOOST_CHECK_EQUAL(resp.places(11).quality(), 90);
}

BOOST_AUTO_TEST_CASE(synonyms_without_grand_champ_test){
    int nbmax = 10;
    autocomplete_map synonyms;
    std::set<std::string> ghostwords;
    synonyms["hotel de ville"]="mairie";
    synonyms["cc"]="centre commercial";
    synonyms["gare sncf"]="gare";
    synonyms["bd"]="boulevard";
    synonyms["bld"]="boulevard";
    synonyms["bvd"]="boulevard";
    synonyms["chr"]="hopital";
    synonyms["chu"]="hopital";
    synonyms["ld"]="lieu-dit";
    synonyms["pt"]="pont";
    synonyms["rle"]="ruelle";
    synonyms["rte"]="route";
    synonyms["sq"]="square";
    synonyms["st"]="saint";
    synonyms["ste"]="sainte";
    synonyms["vla"]="villa";

    Autocomplete<unsigned int> ac;
    ac.add_string("Grand-Champ 56390", 0, ghostwords, synonyms);
    ac.add_string("Locmaria-Grand-Champ 56390", 1, ghostwords, synonyms);
    ac.add_string("Place de la Mairie Grand-Champ", 2, ghostwords, synonyms);
    ac.add_string("Champs-Elysées - Clémenceau - Grand Palais Paris", 3, ghostwords, synonyms);
    ac.add_string("Collec Locmaria-Grand-Champ", 4, ghostwords, synonyms);
    ac.add_string("Grandchamp 52600", 5, ghostwords, synonyms);
    ac.add_string("Parking Grandchamp Grandchamp", 6, ghostwords, synonyms);
    ac.add_string("impasse DE GRANDCHAMP Trégunc", 7, ghostwords, synonyms);
    ac.build();

    //search : "grand-champ" -> "grand champ" no synonym is applied for search string
    //Found : "Grand-Champ 56390", "Locmaria-Grand-Champ 56390", "Place de la Mairie Grand-Champ" ,
    //"Champs-Elysées - Clémenceau - Grand Palais Paris" et "Collec Locmaria-Grand-Champ"
    auto res = ac.find_complete("grand-champ", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res.size(), 5);

    //search : "grandchamp" -> "grandchamp" no synonym is applied for search string
    //Found : "Grandchamp 52600", "Parking Grandchamp Grandchamp" et "impasse DE GRANDCHAMP Trégunc"
    auto res1 = ac.find_complete("grandchamp", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res1.size(), 3);
}

BOOST_AUTO_TEST_CASE(synonyms_with_grand_champ_test){
    int nbmax = 10;
    autocomplete_map synonyms;
    std::set<std::string> ghostwords;
    synonyms["hotel de ville"]="mairie";
    synonyms["cc"]="centre commercial";
    synonyms["gare sncf"]="gare";
    synonyms["bd"]="boulevard";
    synonyms["bld"]="boulevard";
    synonyms["bvd"]="boulevard";
    synonyms["chr"]="hopital";
    synonyms["chu"]="hopital";
    synonyms["ld"]="lieu-dit";
    synonyms["pt"]="pont";
    synonyms["rle"]="ruelle";
    synonyms["rte"]="route";
    synonyms["sq"]="square";
    synonyms["st"]="saint";
    synonyms["ste"]="sainte";
    synonyms["vla"]="villa";
    synonyms["grand-champ"]="grandchamp";

    Autocomplete<unsigned int> ac;
    ac.add_string("Grand-Champ 56390", 0, ghostwords, synonyms);
    ac.add_string("Locmaria-Grand-Champ 56390", 1, ghostwords, synonyms);
    ac.add_string("Place de la Mairie Grand-Champ", 2, ghostwords, synonyms);
    ac.add_string("Champs-Elysées - Clémenceau - Grand Palais Paris", 3, ghostwords, synonyms);
    ac.add_string("Collec Locmaria-Grand-Champ", 4, ghostwords, synonyms);
    ac.add_string("Grandchamp 52600", 5, ghostwords, synonyms);
    ac.add_string("Parking Grandchamp Grandchamp", 6, ghostwords, synonyms);
    ac.add_string("impasse DE GRANDCHAMP Trégunc", 7, ghostwords, synonyms);
    ac.build();

    //search : "grand-champ" -> "grand champ" no synonym is applied for search string
    //Found : all
    auto res = ac.find_complete("grand-champ", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res.size(), 8);

    //search : "grandchamp" -> "grandchamp" no synonym is applied for search string
    //Found : All except "Champs-Elysées - Clémenceau - Grand Palais Paris"
    auto res1 = ac.find_complete("grandchamp", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res1.size(), 7);
}



BOOST_AUTO_TEST_CASE(autocomplete_with_multi_postal_codes_test) {

    std::vector<std::string> admins;
    std::vector<navitia::type::Type_e> type_filter;
    ed::builder b("20140614");
    b.sa("Santec", 0, 0);

    b.data->pt_data->index();
    Admin* ad = new Admin;
    ad->name = "Nantes";
    ad->uri = "URI-NANTES";
    ad->level = 8;
    ad->postal_codes.push_back("44000");
    ad->postal_codes.push_back("44100");
    ad->postal_codes.push_back("44200");
    ad->postal_codes.push_back("44300");
    ad->idx = 0;
    b.data->geo_ref->admins.push_back(ad);
    b.manage_admin();
    b.build_autocomplete();
    b.data->pt_data->stop_area_autocomplete.word_quality_list.at(0).score = 100;


    type_filter.push_back(navitia::type::Type_e::StopArea);
    type_filter.push_back(navitia::type::Type_e::Admin);
    pbnavitia::Response resp = navitia::autocomplete::autocomplete("Sante", type_filter , 1, 15, admins, 0,
                                                                   *(b.data), boost::gregorian::not_a_date_time);

    BOOST_REQUIRE_EQUAL(resp.places_size(), 1);
    BOOST_CHECK_EQUAL(resp.places(0).embedded_type() , pbnavitia::STOP_AREA);
    BOOST_REQUIRE_EQUAL(resp.places(0).quality(), 90);
    BOOST_REQUIRE_EQUAL(resp.places(0).stop_area().administrative_regions().size(), 1);
    BOOST_REQUIRE_EQUAL(resp.places(0).stop_area().administrative_regions(0).label(), "Nantes (44000-44300)");
}

BOOST_AUTO_TEST_CASE(autocomplete_with_multi_postal_codes_alphanumeric_test) {

    std::vector<std::string> admins;
    std::vector<navitia::type::Type_e> type_filter;
    ed::builder b("20140614");
    b.sa("Santec", 0, 0);

    b.data->pt_data->index();
    Admin* ad = new Admin;
    ad->name = "Nantes";
    ad->uri = "URI-NANTES";
    ad->level = 8;
    ad->postal_codes.push_back("44AAA");
    ad->postal_codes.push_back("44100");
    ad->postal_codes.push_back("44200");
    ad->postal_codes.push_back("44300");
    ad->idx = 0;
    b.data->geo_ref->admins.push_back(ad);
    b.manage_admin();
    b.build_autocomplete();
    b.data->pt_data->stop_area_autocomplete.word_quality_list.at(0).score = 100;


    type_filter.push_back(navitia::type::Type_e::StopArea);
    type_filter.push_back(navitia::type::Type_e::Admin);
    pbnavitia::Response resp = navitia::autocomplete::autocomplete("Sante", type_filter , 1, 15, admins, 0,
                                                                   *(b.data), boost::gregorian::not_a_date_time);

    BOOST_REQUIRE_EQUAL(resp.places_size(), 1);
    BOOST_CHECK_EQUAL(resp.places(0).embedded_type() , pbnavitia::STOP_AREA);
    BOOST_REQUIRE_EQUAL(resp.places(0).quality(), 90);
    BOOST_REQUIRE_EQUAL(resp.places(0).stop_area().administrative_regions().size(), 1);
    BOOST_REQUIRE_EQUAL(resp.places(0).stop_area().administrative_regions(0).label(), "Nantes (44AAA;44100;44200;44300)");
}


BOOST_AUTO_TEST_CASE(autocomplete_without_postal_codes_test) {

    std::vector<std::string> admins;
    std::vector<navitia::type::Type_e> type_filter;
    ed::builder b("20140614");
    b.sa("Santec", 0, 0);

    b.data->pt_data->index();
    Admin* ad = new Admin;
    ad->name = "Nantes";
    ad->uri = "URI-NANTES";
    ad->level = 8;
    ad->idx = 0;
    b.data->geo_ref->admins.push_back(ad);
    b.manage_admin();
    b.build_autocomplete();
    b.data->pt_data->stop_area_autocomplete.word_quality_list.at(0).score = 100;


    type_filter.push_back(navitia::type::Type_e::StopArea);
    type_filter.push_back(navitia::type::Type_e::Admin);
    pbnavitia::Response resp = navitia::autocomplete::autocomplete("Sante", type_filter , 1, 15, admins, 0,
                                                                   *(b.data), boost::gregorian::not_a_date_time);

    BOOST_REQUIRE_EQUAL(resp.places_size(), 1);
    BOOST_CHECK_EQUAL(resp.places(0).embedded_type() , pbnavitia::STOP_AREA);
    BOOST_REQUIRE_EQUAL(resp.places(0).quality(), 90);
    BOOST_REQUIRE_EQUAL(resp.places(0).stop_area().administrative_regions().size(), 1);
    BOOST_REQUIRE_EQUAL(resp.places(0).stop_area().administrative_regions(0).label(), "Nantes");
}


BOOST_AUTO_TEST_CASE(autocomplete_with_multi_postal_codes_testAA) {

    std::vector<std::string> admins;
    std::vector<navitia::type::Type_e> type_filter;
    ed::builder b("20140614");

    Admin* ad = new Admin;
    ad->name = "Nantes";
    ad->uri = "URI-NANTES";
    ad->level = 8;
    ad->postal_codes.push_back("44000");
    ad->postal_codes.push_back("44100");
    ad->postal_codes.push_back("44200");
    ad->postal_codes.push_back("44300");
    ad->idx = 0;
    b.data->geo_ref->admins.push_back(ad);

    navitia::georef::Way* w = new navitia::georef::Way();
    w->idx = 0;
    w->name = "Sante";
    w->uri = w->name;
    w->admin_list.push_back(ad);
    b.data->geo_ref->ways.push_back(w);

    ad = new Admin();
    ad->name = "Tours";
    ad->uri = "URI-TOURS";
    ad->level = 8;
    ad->postal_codes.push_back("37000");
    ad->postal_codes.push_back("37100");
    ad->postal_codes.push_back("37200");
    ad->idx = 1;
    b.data->geo_ref->admins.push_back(ad);

    w = new navitia::georef::Way();
    w->idx = 1;
    w->name = "Sante";
    w->uri = w->name;
    w->admin_list.push_back(ad);
    b.data->geo_ref->ways.push_back(w);

    b.build_autocomplete();
    type_filter.push_back(navitia::type::Type_e::Address);
    pbnavitia::Response resp = navitia::autocomplete::autocomplete("Sante 37000", type_filter , 1, 10, admins, 0,
                                                                   *(b.data), boost::gregorian::not_a_date_time);

    BOOST_REQUIRE_EQUAL(resp.places_size(), 1);
    BOOST_CHECK_EQUAL(resp.places(0).embedded_type() , pbnavitia::ADDRESS);
    BOOST_REQUIRE_EQUAL(resp.places(0).quality(), 70);
    BOOST_REQUIRE_EQUAL(resp.places(0).address().administrative_regions().size(), 1);
    BOOST_REQUIRE_EQUAL(resp.places(0).address().administrative_regions(0).label(), "Tours (37000-37200)");
    BOOST_REQUIRE_EQUAL(resp.places(0).address().administrative_regions(0).zip_code(), "37000;37100;37200");

    resp = navitia::autocomplete::autocomplete("Sante 44000", type_filter , 1, 10, admins, 0, *(b.data),
                                               boost::gregorian::not_a_date_time);

    BOOST_REQUIRE_EQUAL(resp.places_size(), 1);
    BOOST_CHECK_EQUAL(resp.places(0).embedded_type() , pbnavitia::ADDRESS);
    BOOST_REQUIRE_EQUAL(resp.places(0).quality(), 60);
    BOOST_REQUIRE_EQUAL(resp.places(0).address().administrative_regions().size(), 1);
    BOOST_REQUIRE_EQUAL(resp.places(0).address().administrative_regions(0).label(), "Nantes (44000-44300)");
    BOOST_REQUIRE_EQUAL(resp.places(0).address().administrative_regions(0).zip_code(), "44000;44100;44200;44300");

    type_filter.clear();
    type_filter.push_back(navitia::type::Type_e::Admin);
    resp = navitia::autocomplete::autocomplete("37000", type_filter , 1, 10, admins, 0, *(b.data),
                                               boost::gregorian::not_a_date_time);
    BOOST_REQUIRE_EQUAL(resp.places_size(), 1);
    BOOST_CHECK_EQUAL(resp.places(0).embedded_type() , pbnavitia::ADMINISTRATIVE_REGION);
    BOOST_REQUIRE_EQUAL(resp.places(0).quality(), 70);
    BOOST_REQUIRE_EQUAL(resp.places(0).administrative_region().label(), "Tours (37000-37200)");
    BOOST_REQUIRE_EQUAL(resp.places(0).administrative_region().zip_code(), "37000;37100;37200");

    resp = navitia::autocomplete::autocomplete("37100", type_filter , 1, 10, admins, 0, *(b.data),
                                               boost::gregorian::not_a_date_time);
    BOOST_REQUIRE_EQUAL(resp.places_size(), 1);
    BOOST_CHECK_EQUAL(resp.places(0).embedded_type() , pbnavitia::ADMINISTRATIVE_REGION);
    BOOST_REQUIRE_EQUAL(resp.places(0).quality(), 70);
    BOOST_REQUIRE_EQUAL(resp.places(0).administrative_region().label(), "Tours (37000-37200)");
    BOOST_REQUIRE_EQUAL(resp.places(0).administrative_region().zip_code(), "37000;37100;37200");

    resp = navitia::autocomplete::autocomplete("37200", type_filter , 1, 10, admins, 0, *(b.data),
                                               boost::gregorian::not_a_date_time);
    BOOST_REQUIRE_EQUAL(resp.places_size(), 1);
    BOOST_CHECK_EQUAL(resp.places(0).embedded_type() , pbnavitia::ADMINISTRATIVE_REGION);
    BOOST_REQUIRE_EQUAL(resp.places(0).quality(), 70);
    BOOST_REQUIRE_EQUAL(resp.places(0).administrative_region().label(), "Tours (37000-37200)");
    BOOST_REQUIRE_EQUAL(resp.places(0).administrative_region().zip_code(), "37000;37100;37200");

}

BOOST_AUTO_TEST_CASE(regex_toknize_with_synonyms_having_ghostword_tests){
    autocomplete_map synonyms;
    std::set<std::string> ghostwords;
    synonyms["chr"]="hopital";
    synonyms["bvd"]="boulevard";
    synonyms["bld"]="boulevard";
    synonyms["bd"]="boulevard";

    ghostwords.insert("de");
    ghostwords.insert("la");

    Autocomplete<unsigned int> ac;
    std::set<std::string> vec;

    vec = ac.tokenize("gare (Baune)", ghostwords ,synonyms);
    BOOST_REQUIRE_EQUAL(vec.size(), 2);
    vec.clear();
    vec = ac.tokenize("rue de la Garenne (Beaune)", ghostwords ,synonyms);
    BOOST_REQUIRE_EQUAL(vec.size(), 3);

    vec.clear();
    vec = ac.tokenize("gare de Baune", ghostwords ,synonyms);
    BOOST_REQUIRE_EQUAL(vec.size(), 2);
    BOOST_CHECK(vec.find("gare") != vec.end());
    BOOST_CHECK(vec.find("baune") != vec.end());
    BOOST_CHECK(vec.find("de") == vec.end());
}

BOOST_AUTO_TEST_CASE(autocomplete_with_ghostword_test) {
    std::vector<std::string> admins;
    std::vector<navitia::type::Type_e> type_filter;
    ed::builder b("20140614");
    autocomplete_map synonyms;
     std::set<std::string> ghostwords;

    synonyms["cc"]="centre commercial";
    synonyms["hotel de ville"]="mairie";
    synonyms["gare sncf"]="gare";
    synonyms["chu"]="hopital";
    synonyms["chr"]="hopital";
    synonyms["ld"]="Lieu-Dit";
    synonyms["st"]="saint";
    synonyms["ste"]="sainte";
    synonyms["bvd"]="Boulevard";
    synonyms["bld"]="Boulevard";
    synonyms["bd"]="Boulevard";
    synonyms["pas"]="Passage";
    synonyms["pt"]="Pont";
    synonyms["rle"]="Ruelle";
    synonyms["rte"]="Route";
    synonyms["vla"]="Villa";

    ghostwords.insert("de");
    ghostwords.insert("la");
    ghostwords.insert("les");
    ghostwords.insert("des");
    ghostwords.insert("le");
    ghostwords.insert("l");
    ghostwords.insert("du");
    ghostwords.insert("d");
    ghostwords.insert("sur");
    ghostwords.insert("au");
    ghostwords.insert("aux");
    ghostwords.insert("en");
    ghostwords.insert("et");

    Admin* ad = new Admin;
    ad->name = "Beaune";
    ad->uri = "URI-Beaune";
    ad->level = 8;
    ad->postal_codes.push_back("21200");
    ad->idx = 0;
    b.data->geo_ref->admins.push_back(ad);

    navitia::georef::Way* w = new navitia::georef::Way();
    w->idx = 0;
    w->name = "place de la Gare";
    w->uri = w->name;
    w->admin_list.push_back(ad);
    b.data->geo_ref->ways.push_back(w);

    w = new navitia::georef::Way();
    w->idx = 1;
    w->name = "rue de la Garenne";
    w->uri = w->name;
    w->admin_list.push_back(ad);
    b.data->geo_ref->ways.push_back(w);

    //Create a new StopArea
    navitia::type::StopArea* sa = new navitia::type::StopArea();
    sa->idx = 0;
    sa->name = "gare";
    sa->uri = sa->name;
    sa->admin_list.push_back(ad);
    b.data->pt_data->stop_areas.push_back(sa);

    b.data->geo_ref->synonyms = synonyms;
    b.data->geo_ref->ghostwords = ghostwords;
    b.build_autocomplete();
    type_filter.push_back(navitia::type::Type_e::Address);
    type_filter.push_back(navitia::type::Type_e::Admin);
    type_filter.push_back(navitia::type::Type_e::StopArea);

    pbnavitia::Response resp = navitia::autocomplete::autocomplete("gare beaune", type_filter , 1, 10, admins, 0,
                                                                   *(b.data), boost::gregorian::not_a_date_time);

    BOOST_REQUIRE_EQUAL(resp.places_size(), 3);
    BOOST_CHECK_EQUAL(resp.places(0).embedded_type() , pbnavitia::STOP_AREA);
    BOOST_REQUIRE_EQUAL(resp.places(0).stop_area().administrative_regions().size(), 1);
    BOOST_REQUIRE_EQUAL(resp.places(0).stop_area().administrative_regions(0).label(), "Beaune (21200)");
    BOOST_REQUIRE_EQUAL(resp.places(0).stop_area().administrative_regions(0).zip_code(), "21200");

    resp = navitia::autocomplete::autocomplete("gare de beaune", type_filter , 1, 10, admins, 0,
                                               *(b.data), boost::gregorian::not_a_date_time);
    BOOST_REQUIRE_EQUAL(resp.places_size(), 3);
    BOOST_CHECK_EQUAL(resp.places(0).embedded_type() , pbnavitia::STOP_AREA);
    BOOST_REQUIRE_EQUAL(resp.places(0).stop_area().administrative_regions().size(), 1);
    BOOST_REQUIRE_EQUAL(resp.places(0).stop_area().administrative_regions(0).label(), "Beaune (21200)");
    BOOST_REQUIRE_EQUAL(resp.places(0).stop_area().administrative_regions(0).zip_code(), "21200");
}

BOOST_AUTO_TEST_CASE(regex_toknize_with_synonyms_and_ghostwords_tests){

    autocomplete_map synonyms;
    synonyms["cc"]="centre commercial";
    synonyms["hotel de ville"]="mairie";
    synonyms["gare sncf"]="gare";
    synonyms["chu"]="hopital";
    synonyms["chr"]="hopital";
    synonyms["ld"]="Lieu-Dit";
    synonyms["ste"]="sainte";
    synonyms["bvd"]="Boulevard";
    synonyms["pas"]="Passage";
    synonyms["pt"]="Pont";
    synonyms["vla"]="Villa";

    std::set<std::string> ghostwords;

    ghostwords.insert("de");
    ghostwords.insert("la");
    ghostwords.insert("les");
    ghostwords.insert("des");
    ghostwords.insert("le");
    ghostwords.insert("l");

    Autocomplete<unsigned int> ac;
    std::set<std::string> vec;

    //synonyme : "cc" = "centre commercial" / synonym : de = ""
    //"cc Carré de Soie" -> "cc centre commercial carré soie"
    vec = ac.tokenize("cc Carré de Soie", ghostwords, synonyms);
    BOOST_CHECK_EQUAL(vec.size(),5);
    BOOST_CHECK(vec.find("carre") != vec.end());
    BOOST_CHECK(vec.find("centre") != vec.end());
    BOOST_CHECK(vec.find("commercial") != vec.end());
    BOOST_CHECK(vec.find("soie") != vec.end());
    BOOST_CHECK(vec.find("cc") != vec.end());

    vec.clear();
    //synonyme : "cc" = "centre commercial" / synonym : de = ""
    //"cc de Carré de Soie" -> "cc centre commercial carré soie"
    vec = ac.tokenize("cc de Carré de Soie", ghostwords, synonyms);
    BOOST_CHECK_EQUAL(vec.size(),5);
    BOOST_CHECK(vec.find("carre") != vec.end());
    BOOST_CHECK(vec.find("centre") != vec.end());
    BOOST_CHECK(vec.find("commercial") != vec.end());
    BOOST_CHECK(vec.find("soie") != vec.end());
    BOOST_CHECK(vec.find("cc") != vec.end());

    vec.clear();
    //"mairie" -> "mairie hotel ville"
    vec = ac.tokenize("mairie", ghostwords, synonyms);
    BOOST_CHECK_EQUAL(vec.size(),3);
    BOOST_CHECK(vec.find("mairie") != vec.end());
    BOOST_CHECK(vec.find("hotel") != vec.end());
    BOOST_CHECK(vec.find("ville") != vec.end());

    vec.clear();
    //"mairie de beaume" -> "mairie hotel ville beaume"
    vec = ac.tokenize("mairie de beaume", ghostwords, synonyms);
    BOOST_CHECK_EQUAL(vec.size(),4);
    BOOST_CHECK(vec.find("mairie") != vec.end());
    BOOST_CHECK(vec.find("hotel") != vec.end());
    BOOST_CHECK(vec.find("ville") != vec.end());
    BOOST_CHECK(vec.find("beaume") != vec.end());
}


BOOST_AUTO_TEST_CASE(synonyms_with_non_ascii){
    int nbmax = 10;
    std::set<std::string> ghostwords{};

    autocomplete_map synonyms{
        {"fac", "université"},
        {"faculté", "université"},
        {"embarcadère", "gare maritime"}
    };

    Autocomplete<unsigned int> ac;
    ac.add_string("université de canaltp", 0, ghostwords, synonyms);
    ac.add_string("gare maritime de canaltp", 1, ghostwords, synonyms);
    ac.build();

    auto res0 = ac.find_complete("fac", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res0.size(), 1);

    auto res1 = ac.find_complete("faculté", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res2 = ac.find_complete("embarca", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res2.size(), 1);

}

BOOST_AUTO_TEST_CASE(synonyms_with_capital){
    int nbmax = 10;
    std::set<std::string> ghostwords{};
    autocomplete_map synonyms{{"ANPE", "Pole Emploi"}};

    Autocomplete<unsigned int> ac;
    ac.add_string("Pole Emploi de paris", 0, ghostwords, synonyms);
    ac.build();

    auto res0 = ac.find_complete("ANPE", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res0.size(), 1);

    auto res1 = ac.find_complete("anpe", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);

    auto res2 = ac.find_complete("AnPe", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res2.size(), 1);

}

BOOST_AUTO_TEST_CASE(synonyms_with_capital_and_non_ascii){
    int nbmax = 10;
    std::set<std::string> ghostwords{};
    autocomplete_map synonyms{
        {"CPAM", "sécurité sociale"}
    };

    Autocomplete<unsigned int> ac;
    ac.add_string("Sécurité Sociale de paris", 0, ghostwords, synonyms);
    ac.build();

    auto res0 = ac.find_complete("CPAM", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res0.size(), 1);

    auto res1 = ac.find_complete("cpam", nbmax, [](int){return true;}, ghostwords);
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
}

/*
 * Test the admin filtering
 *
 * We have 2 stops:
 *   * bob in BobBille
 *   * bobette' not in 'BobVille'
 *
 * If we autocomplete for "bob", we should have BobVille + bob + bobette
 *
 * If we autocomplete for "bob" but only for places in Bobville, we should have BobVille + bob
 */
BOOST_AUTO_TEST_CASE(autocomplete_admin_filtering_tests) {
    ed::builder b("20140614");

    Admin* bobville = new Admin;
    bobville->name = "BobVille";
    bobville->uri = "BobVille";
    bobville->level = 8;
    bobville->postal_codes.push_back("29000");
    bobville->idx = 0;
    b.data->geo_ref->admins.push_back(bobville);

    auto* bob = b.sa("bob", 0, 0).sa;
    bob->admin_list.push_back(bobville);
    b.sa("bobette", 0, 0);

    b.data->pt_data->index();
    b.build_autocomplete();

    std::vector<navitia::type::Type_e> type_filter {
        navitia::type::Type_e::StopArea,
        navitia::type::Type_e::Admin
    };

    std::vector<std::string> admins = {""}; // no filtering on the admin
    auto resp = navitia::autocomplete::autocomplete("bob", type_filter, 1, 10, admins, 0, *(b.data),
                                                    boost::gregorian::not_a_date_time);

    BOOST_REQUIRE_EQUAL(resp.places_size(), 3);
    BOOST_CHECK_EQUAL(resp.places(0).uri(), "BobVille");
    BOOST_CHECK_EQUAL(resp.places(1).uri(), "bobette");
    BOOST_CHECK_EQUAL(resp.places(2).uri(), "bob");

    admins = {"BobVille"};
    resp = navitia::autocomplete::autocomplete("bob", type_filter, 1, 10, admins, 0, *(b.data),
                                               boost::gregorian::not_a_date_time);

    BOOST_REQUIRE_EQUAL(resp.places_size(), 2);
    // we should only find the admin and bob
    BOOST_CHECK_EQUAL(resp.places(0).uri(), "BobVille");
    BOOST_CHECK_EQUAL(resp.places(1).uri(), "bob");
}
