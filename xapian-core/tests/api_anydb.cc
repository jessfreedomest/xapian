/* api_anydb.cc: tests which work with any backend
 *
 * ----START-LICENCE----
 * Copyright 1999,2000,2001 BrightStation PLC
 * Copyright 2002 Ananova Ltd
 * Copyright 2002,2003,2004 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 * -----END-LICENCE-----
 */

#include <config.h>
#include <algorithm>
#include <string>

#include <xapian.h>
#include "testsuite.h"
#include "testutils.h"
#include "utils.h"

#include "apitest.h"
#include "api_wrdb.h"

#include <list>

using namespace std;

typedef list<string> om_termname_list;

static void
print_mset_weights(const Xapian::MSet &mset)
{
    Xapian::MSetIterator i = mset.begin();
    for ( ; i != mset.end(); ++i) {
        tout << " " << i.get_weight();
    }
}

static void
print_mset_percentages(const Xapian::MSet &mset)
{
    Xapian::MSetIterator i = mset.begin();
    for ( ; i != mset.end(); ++i) {
        tout << " " << mset.convert_to_percent(i);
    }
}

static Xapian::MSet
do_get_simple_query_mset(Xapian::Query query, int maxitems = 10, int first = 0)
{
    // open the database (in this case a simple text file
    // we prepared earlier)
    Xapian::Enquire enquire(get_database("apitest_simpledata"));
    enquire.set_query(query);

    // retrieve the top results
    return enquire.get_mset(first, maxitems);
}

static Xapian::Query
query(Xapian::Query::op op, string t1 = "", string t2 = "",
      string t3 = "", string t4 = "", string t5 = "",
      string t6 = "", string t7 = "", string t8 = "",
      string t9 = "", string t10 = "")
{
    vector<string> v;
    Xapian::Stem stemmer("english");    
    if (!t1.empty()) v.push_back(stemmer.stem_word(t1));
    if (!t2.empty()) v.push_back(stemmer.stem_word(t2));
    if (!t3.empty()) v.push_back(stemmer.stem_word(t3));
    if (!t4.empty()) v.push_back(stemmer.stem_word(t4));
    if (!t5.empty()) v.push_back(stemmer.stem_word(t5));
    if (!t6.empty()) v.push_back(stemmer.stem_word(t6));
    if (!t7.empty()) v.push_back(stemmer.stem_word(t7));
    if (!t8.empty()) v.push_back(stemmer.stem_word(t8));
    if (!t9.empty()) v.push_back(stemmer.stem_word(t9));
    if (!t10.empty()) v.push_back(stemmer.stem_word(t10));
    return Xapian::Query(op, v.begin(), v.end());
}

static Xapian::Query
query(const string &t)
{
    return Xapian::Query(Xapian::Stem("english").stem_word(t));
}

// #######################################################################
// # Tests start here

// tests that the backend doesn't return zero docids
static bool test_zerodocid1()
{
    // open the database (in this case a simple text file
    // we prepared earlier)

    Xapian::Database mydb(get_database("apitest_onedoc"));

    Xapian::Enquire enquire(mydb);

    // make a simple query, with one word in it - "word".
    enquire.set_query(Xapian::Query("word"));

    // retrieve the top ten results (we only expect one)
    Xapian::MSet mymset = enquire.get_mset(0, 10);

    // We've done the query, now check that the result is what
    // we expect (1 document, with non-zero docid)
    TEST_MSET_SIZE(mymset, 1);

    TEST_AND_EXPLAIN(*(mymset.begin()) != 0,
		     "A query on a database returned a zero docid");

    return true;
}

// tests that an empty query returns no matches
static bool test_emptyquery1()
{
    Xapian::MSet mymset = do_get_simple_query_mset(Xapian::Query());
    TEST_MSET_SIZE(mymset, 0);
    TEST_EQUAL(mymset.get_matches_lower_bound(), 0);
    TEST_EQUAL(mymset.get_matches_upper_bound(), 0);
    TEST_EQUAL(mymset.get_matches_estimated(), 0);
    vector<Xapian::Query> v;
    mymset = do_get_simple_query_mset(Xapian::Query(Xapian::Query::OP_AND,
					      v.begin(), v.end()));
    TEST_MSET_SIZE(mymset, 0);
    TEST_EQUAL(mymset.get_matches_lower_bound(), 0);
    TEST_EQUAL(mymset.get_matches_upper_bound(), 0);
    TEST_EQUAL(mymset.get_matches_estimated(), 0);
    return true;
}

// tests the document count for a simple query
static bool test_simplequery1()
{
    Xapian::MSet mymset = do_get_simple_query_mset(Xapian::Query("word"));
    TEST_MSET_SIZE(mymset, 2);
    return true;
}

// tests for the right documents and weights returned with simple query
static bool test_simplequery2()
{
    // open the database (in this case a simple text file
    // we prepared earlier)
    Xapian::Database db = get_database("apitest_simpledata");
    Xapian::Enquire enquire(db);
    enquire.set_query(Xapian::Query("word"));

    // retrieve the top results
    Xapian::MSet mymset = enquire.get_mset(0, 10);

    // We've done the query, now check that the result is what
    // we expect (documents 2 and 4)
    mset_expect_order(mymset, 2, 4);

    // Check the weights
    Xapian::MSetIterator i = mymset.begin();
    if (db.get_avlength() != 1.0) {
	// These weights are for BM25Weight(1,0,1,0.5,0.5)
	weights_are_equal_enough(i.get_weight(), 1.046482);
	i++;
	weights_are_equal_enough(i.get_weight(), 0.640988);
    } else {
	// These weights are for BM25Weight(1,0,1,0.5,0.5) when all
	// doclengths are reported as 1 (e.g. muscat36 backend)
	weights_are_equal_enough(i.get_weight(), 0.641854);
	i++;
	weights_are_equal_enough(i.get_weight(), 0.641854);
    }

    return true;
}

// tests for the right document count for another simple query
static bool test_simplequery3()
{
    Xapian::MSet mymset = do_get_simple_query_mset(query("this"));

    // Check that 6 documents were returned.
    TEST_MSET_SIZE(mymset, 6);

    return true;
}

// tests a query across multiple databases
static bool test_multidb1()
{
    Xapian::Database mydb1(get_database("apitest_simpledata", "apitest_simpledata2"));
    Xapian::Enquire enquire1(mydb1);

    Xapian::Database mydb2(get_database("apitest_simpledata"));
    mydb2.add_database(get_database("apitest_simpledata2"));
    Xapian::Enquire enquire2(mydb2);

    // make a simple query, with one word in it - "word".
    Xapian::Query myquery("word");
    enquire1.set_query(myquery);
    enquire2.set_query(myquery);

    // retrieve the top ten results from each method of accessing
    // multiple text files
    Xapian::MSet mymset1 = enquire1.get_mset(0, 10);
    Xapian::MSet mymset2 = enquire2.get_mset(0, 10);

    TEST_EQUAL(mymset1.size(), mymset2.size());
    TEST(mset_range_is_same_weights(mymset1, 0, mymset2, 0, mymset1.size()));
    return true;
}

// tests a query across multiple databases with terms only
// in one of the two databases
static bool test_multidb2()
{
    Xapian::Database mydb1(get_database("apitest_simpledata",
				  "apitest_simpledata2"));
    Xapian::Enquire enquire1(mydb1);

    Xapian::Database mydb2(get_database("apitest_simpledata"));
    mydb2.add_database(get_database("apitest_simpledata2"));
    Xapian::Enquire enquire2(mydb2);

    // make a simple query
    Xapian::Query myquery = query(Xapian::Query::OP_OR, "inmemory", "word");
    enquire1.set_query(myquery);
    enquire2.set_query(myquery);

    // retrieve the top ten results from each method of accessing
    // multiple text files
    Xapian::MSet mymset1 = enquire1.get_mset(0, 10);
    Xapian::MSet mymset2 = enquire2.get_mset(0, 10);

    TEST_EQUAL(mymset1.size(), mymset2.size());
    TEST(mset_range_is_same_weights(mymset1, 0, mymset2, 0, mymset1.size()));
    return true;
}

// test that a multidb with 2 dbs query returns correct docids
static bool test_multidb3()
{
    Xapian::Database mydb2(get_database("apitest_simpledata"));
    mydb2.add_database(get_database("apitest_simpledata2"));
    Xapian::Enquire enquire(mydb2);

    // make a query
    Xapian::Query myquery = query(Xapian::Query::OP_OR, "inmemory", "word");
    enquire.set_weighting_scheme(Xapian::BoolWeight());
    enquire.set_query(myquery);

    // retrieve the top ten results
    Xapian::MSet mymset = enquire.get_mset(0, 10);
    mset_expect_order(mymset, 2, 3, 7);

    return true;
}

// test that a multidb with 3 dbs query returns correct docids
static bool test_multidb4()
{
    Xapian::Database mydb2(get_database("apitest_simpledata"));
    mydb2.add_database(get_database("apitest_simpledata2"));
    mydb2.add_database(get_database("apitest_termorder"));
    Xapian::Enquire enquire(mydb2);

    // make a query
    Xapian::Query myquery = query(Xapian::Query::OP_OR, "inmemory", "word");
    enquire.set_weighting_scheme(Xapian::BoolWeight());
    enquire.set_query(myquery);

    // retrieve the top ten results
    Xapian::MSet mymset = enquire.get_mset(0, 10);
    mset_expect_order(mymset, 2, 3, 4, 10);

    return true;
}

// tests MultiPostList::skip_to().
static bool test_multidb5()
{
    Xapian::Database mydb2(get_database("apitest_simpledata"));
    mydb2.add_database(get_database("apitest_simpledata2"));
    Xapian::Enquire enquire(mydb2);

    // make a query
    Xapian::Query myquery = query(Xapian::Query::OP_AND, "inmemory", "word");
    enquire.set_weighting_scheme(Xapian::BoolWeight());
    enquire.set_query(myquery);

    // retrieve the top ten results
    Xapian::MSet mymset = enquire.get_mset(0, 10);
    mset_expect_order(mymset, 2);

    return true;
}

// tests that changing a query object after calling set_query()
// doesn't make any difference to get_mset().
static bool test_changequery1()
{
    SKIP_TEST("Enquire::set_query() is buggy"); // Need to fix Enquire::set_query()

    // Open the database (in this case a simple text file
    // we prepared earlier)
    Xapian::Database db(get_database("apitest_simpledata"));

    const char * phrase[] = { "this", "paragraph" };
    Xapian::MSet mset1;
    {
	Xapian::Enquire enquire(db);

	// make a simple query
	Xapian::Query myquery(Xapian::Query::OP_NEAR, phrase, phrase + 2);
	enquire.set_query(myquery);

	// retrieve the top ten results
	mset1 = enquire.get_mset(0, 10);
    }

    Xapian::MSet mset2;
    {
	Xapian::Enquire enquire(db);

	// make a simple query
	Xapian::Query myquery(Xapian::Query::OP_NEAR, phrase, phrase + 2);
	enquire.set_query(myquery);

	// Now change the query - this shouldn't affect the query enquire
	// will run.
	myquery.set_window(10);

	// retrieve the top ten results
	mset2 = enquire.get_mset(0, 10);
    }

    // verify that both msets are identical
    TEST_EQUAL(mset1, mset2);
    return true;
}

// tests that when specifiying maxitems to get_mset, no more than
// that are returned.
static bool test_msetmaxitems1()
{
    Xapian::MSet mymset = do_get_simple_query_mset(query("this"), 1);
    TEST_MSET_SIZE(mymset, 1);
    return true;
}

// tests that when specifying maxitems to get_eset, no more than
// that are returned.
static bool test_expandmaxitems1()
{
    Xapian::Enquire enquire(get_database("apitest_simpledata"));
    enquire.set_query(Xapian::Query("this"));

    Xapian::MSet mymset = enquire.get_mset(0, 10);
    tout << "mymset.size() = " << mymset.size() << endl;
    TEST(mymset.size() >= 2);

    Xapian::RSet myrset;
    Xapian::MSetIterator i = mymset.begin();
    myrset.add_document(*i);
    myrset.add_document(*(++i));

    Xapian::ESet myeset = enquire.get_eset(1, myrset);
    TEST_EQUAL(myeset.size(), 1);

    return true;
}

// tests that a pure boolean query has all weights set to 0
static bool test_boolquery1()
{
    Xapian::Query myboolquery(query("this"));

    // open the database (in this case a simple text file
    // we prepared earlier)
    Xapian::Enquire enquire(get_database("apitest_simpledata"));
    enquire.set_query(myboolquery);
    enquire.set_weighting_scheme(Xapian::BoolWeight());

    // retrieve the top results
    Xapian::MSet mymset = enquire.get_mset(0, 10);

    TEST_NOT_EQUAL(mymset.size(), 0);
    TEST_EQUAL(mymset.get_max_possible(), 0);
    for (Xapian::MSetIterator i = mymset.begin(); i != mymset.end(); ++i) {
	TEST_EQUAL(i.get_weight(), 0);
    }
    return true;
}

// tests that get_mset() specifying "this" works as expected
static bool test_msetfirst1()
{
    Xapian::MSet mymset1 = do_get_simple_query_mset(query("this"), 6, 0);
    Xapian::MSet mymset2 = do_get_simple_query_mset(query("this"), 3, 3);
    TEST(mset_range_is_same(mymset1, 3, mymset2, 0, 3));
    return true;
}

// Regression test - we weren't adjusting the index into items[] by firstitem
// in api/omenquire.cc.
static bool test_msetfirst2()
{
    Xapian::MSet mymset1 = do_get_simple_query_mset(query("this"), 6, 0);
    Xapian::MSet mymset2 = do_get_simple_query_mset(query("this"), 3, 3);
    TEST_EQUAL(mymset1[5].get_document().get_data(),
	       mymset2[2].get_document().get_data());
    return true;
}

// tests the converting-to-percent functions
static bool test_topercent1()
{
    Xapian::MSet mymset = do_get_simple_query_mset(query("this"), 20, 0);

    int last_pct = 100;
    Xapian::MSetIterator i = mymset.begin();
    for ( ; i != mymset.end(); ++i) {
	int pct = mymset.convert_to_percent(i);
	TEST_AND_EXPLAIN(pct == i.get_percent(),
			 "convert_to_%(msetitor) != convert_to_%(wt)");
	TEST_AND_EXPLAIN(pct == mymset.convert_to_percent(i.get_weight()),
			 "convert_to_%(msetitor) != convert_to_%(wt)");
        TEST_AND_EXPLAIN(pct >= 0 && pct <= 100,
			 "percentage out of range: " << pct);
        TEST_AND_EXPLAIN(pct <= last_pct, "percentage increased down mset");
	last_pct = pct;
    }
    return true;
}

class myExpandFunctor : public Xapian::ExpandDecider {
    public:
	int operator()(const string & tname) const {
	    unsigned long sum = 0;
	    for (string::const_iterator i=tname.begin(); i!=tname.end(); ++i) {
		sum += *i;
	    }
//	    if (verbose) {
//		tout << tname << "==> " << sum << "\n";
//	    }
	    return (sum % 2) == 0;
	}
};

// tests the expand decision functor
static bool test_expandfunctor1()
{
    Xapian::Enquire enquire(get_database("apitest_simpledata"));
    enquire.set_query(Xapian::Query("this"));

    Xapian::MSet mymset = enquire.get_mset(0, 10);
    TEST(mymset.size() >= 2);

    Xapian::RSet myrset;
    Xapian::MSetIterator i = mymset.begin();
    myrset.add_document(*i);
    myrset.add_document(*(++i));

    myExpandFunctor myfunctor;

    Xapian::ESet myeset_orig = enquire.get_eset(1000, myrset);
    unsigned int neweset_size = 0;
    Xapian::ESetIterator j = myeset_orig.begin();
    for ( ; j != myeset_orig.end(); ++j) {
        if (myfunctor(*j)) neweset_size++;
    }
    Xapian::ESet myeset = enquire.get_eset(neweset_size, myrset, &myfunctor);

#if 0
    // Compare myeset with the hand-filtered version of myeset_orig.
    if (verbose) {
	tout << "orig_eset: ";
	copy(myeset_orig.begin(), myeset_orig.end(),
	     ostream_iterator<Xapian::ESetItem>(tout, " "));
	tout << "\n";

	tout << "new_eset: ";
	copy(myeset.begin(), myeset.end(),
	     ostream_iterator<Xapian::ESetItem>(tout, " "));
	tout << "\n";
    }
#endif
    Xapian::ESetIterator orig = myeset_orig.begin();
    Xapian::ESetIterator filt = myeset.begin();
    for (; orig != myeset_orig.end() && filt != myeset.end(); ++orig, ++filt) {
	// skip over items that shouldn't be in myeset
	while (orig != myeset_orig.end() && !myfunctor(*orig)) {
	    ++orig;
	}

	TEST_AND_EXPLAIN(*orig == *filt &&
			 orig.get_weight() == filt.get_weight(),
			 "Mismatch in items " << *orig << " vs. " << *filt
			 << " after filtering");
    }

    while (orig != myeset_orig.end() && !myfunctor(*orig)) {
	++orig;
    }

    TEST_EQUAL(orig, myeset_orig.end());
    TEST_AND_EXPLAIN(filt == myeset.end(),
		     "Extra items in the filtered eset.");
    return true;
}

// tests the percent cutoff option
static bool test_pctcutoff1()
{
    Xapian::Enquire enquire(get_database("apitest_simpledata"));
    enquire.set_query(query(Xapian::Query::OP_OR,
			    "this", "line", "paragraph", "rubbish"));
    Xapian::MSet mymset1 = enquire.get_mset(0, 100);

    if (verbose) {
	tout << "Original mset pcts:";
	print_mset_percentages(mymset1);
	tout << "\n";
    }

    unsigned int num_items = 0;
    int my_pct = 100;
    int changes = 0;
    Xapian::MSetIterator i = mymset1.begin();
    int c = 0;
    for ( ; i != mymset1.end(); ++i, ++c) {
        int new_pct = mymset1.convert_to_percent(i);
        if (new_pct != my_pct) {
	    changes++;
	    if (changes > 3) break;
	    num_items = c;
	    my_pct = new_pct;
	}
    }

    TEST_AND_EXPLAIN(changes > 3, "MSet not varied enough to test");
    if (verbose) {
        tout << "Cutoff percent: " << my_pct << "\n";
    }

    enquire.set_cutoff(my_pct);
    Xapian::MSet mymset2 = enquire.get_mset(0, 100);

    if (verbose) {
        tout << "Percentages after cutoff:";
	print_mset_percentages(mymset2);
        tout << "\n";
    }

    TEST_AND_EXPLAIN(mymset2.size() >= num_items,
		     "Match with % cutoff lost too many items");

    TEST_AND_EXPLAIN(mymset2.size() == num_items ||
		     (mymset2.convert_to_percent(mymset2[num_items]) == my_pct &&
		      mymset2.convert_to_percent(mymset2.back()) == my_pct),
		     "Match with % cutoff returned too many items");

    return true;
}

// tests the cutoff option
static bool test_cutoff1()
{
    Xapian::Enquire enquire(get_database("apitest_simpledata"));
    enquire.set_query(query(Xapian::Query::OP_OR,
			    "this", "line", "paragraph", "rubbish"));
    Xapian::MSet mymset1 = enquire.get_mset(0, 100);

    if (verbose) {
	tout << "Original mset weights:";
	print_mset_weights(mymset1);
	tout << "\n";
    }

    unsigned int num_items = 0;
    Xapian::weight my_wt = -100;
    int changes = 0;
    Xapian::MSetIterator i = mymset1.begin();
    int c = 0;
    for ( ; i != mymset1.end(); ++i, ++c) {
        Xapian::weight new_wt = i.get_weight();
        if (new_wt != my_wt) {
	    changes++;
	    if (changes > 3) break;
	    num_items = c;
	    my_wt = new_wt;
	}
    }

    TEST_AND_EXPLAIN(changes > 3, "MSet not varied enough to test");
    if (verbose) {
        tout << "Cutoff weight: " << my_wt << "\n";
    }

    enquire.set_cutoff(0, my_wt);
    Xapian::MSet mymset2 = enquire.get_mset(0, 100);

    if (verbose) {
        tout << "Weights after cutoff:";
	print_mset_weights(mymset2);
        tout << "\n";
    }

    TEST_AND_EXPLAIN(mymset2.size() >= num_items,
		     "Match with cutoff lost too many items");

    TEST_AND_EXPLAIN(mymset2.size() == num_items ||
		     (mymset2[num_items].get_weight() == my_wt &&
		      mymset2.back().get_weight() == my_wt),
		     "Match with cutoff returned too many items");

    return true;
}

// tests the cutoff option
static bool test_cutoff2()
{
    Xapian::Enquire enquire(get_database("apitest_simpledata"));
    Xapian::Query q = query(Xapian::Query::OP_OR, "this", "line", "paragraph", "rubbish");
    enquire.set_query(query(Xapian::Query::OP_OR,
			    "this", "line", "paragraph", "rubbish"));
    Xapian::MSet mymset1 = enquire.get_mset(0, 100);

    if (verbose) {
	tout << "Original mset weights:";
	print_mset_weights(mymset1);
	tout << "\n";
    }

    unsigned int num_items = 0;
    Xapian::weight my_wt = -100;
    int changes = 0;
    Xapian::MSetIterator i = mymset1.begin();
    int c = 0;
    for ( ; i != mymset1.end(); ++i, ++c) {
        Xapian::weight new_wt = i.get_weight();
        if (new_wt != my_wt) {
	    changes++;
	    if (changes > 3) break;
	    num_items = c;
	    my_wt = new_wt;
	}
    }

    TEST_AND_EXPLAIN(changes > 3, "MSet not varied enough to test");
    if (verbose) {
        tout << "Cutoff weight: " << my_wt << "\n";
    }

    Xapian::Query cutoffq(Xapian::Query::OP_WEIGHT_CUTOFF, q);
    cutoffq.set_cutoff(my_wt);
    enquire.set_query(cutoffq);
    Xapian::MSet mymset2 = enquire.get_mset(0, 100);

    if (verbose) {
        tout << "Weights after cutoff:";
	print_mset_weights(mymset2);
        tout << "\n";
    }

    TEST_AND_EXPLAIN(mymset2.size() >= num_items,
		     "Match with cutoff lost too many items");

    TEST_AND_EXPLAIN(mymset2.size() == num_items ||
		     (mymset2[num_items].get_weight() == my_wt &&
		      mymset2.back().get_weight() == my_wt),
		     "Match with cutoff returned too many items");

    return true;
}

// tests the allow query terms expand option
static bool test_allowqterms1()
{
    Xapian::Enquire enquire(get_database("apitest_simpledata"));
    enquire.set_query(Xapian::Query("this"));

    Xapian::MSet mymset = enquire.get_mset(0, 10);
    TEST(mymset.size() >= 2);

    Xapian::RSet myrset;
    Xapian::MSetIterator i = mymset.begin();
    myrset.add_document(*i);
    myrset.add_document(*(++i));

    Xapian::ESet myeset = enquire.get_eset(1000, myrset);
    Xapian::ESetIterator j = myeset.begin();
    for ( ; j != myeset.end(); ++j) {
        TEST_NOT_EQUAL(*j, "this");
    }

    Xapian::ESet myeset2 = enquire.get_eset(1000, myrset, Xapian::Enquire::include_query_terms);
    j = myeset2.begin();
    for ( ; j != myeset2.end(); ++j) {
        if (*j == "this") break;
    }
    TEST(j != myeset2.end());
    return true;
}

// tests that the MSet max_attained works
static bool test_maxattain1()
{
    Xapian::MSet mymset = do_get_simple_query_mset(query("this"), 100, 0);

    Xapian::weight mymax = 0;
    Xapian::MSetIterator i = mymset.begin();
    for ( ; i != mymset.end(); ++i) {
        if (i.get_weight() > mymax) mymax = i.get_weight();
    }
    TEST_EQUAL(mymax, mymset.get_max_attained());

    return true;
}

// tests a reversed boolean query
static bool test_reversebool1()
{
    Xapian::Enquire enquire(get_database("apitest_simpledata"));
    enquire.set_query(Xapian::Query("this"));
    enquire.set_weighting_scheme(Xapian::BoolWeight());

    Xapian::MSet mymset1 = enquire.get_mset(0, 100);
    TEST_AND_EXPLAIN(mymset1.size() > 1,
		     "Mset was too small to test properly");

    enquire.set_sort_forward(true);
    Xapian::MSet mymset2 = enquire.get_mset(0, 100);
    enquire.set_sort_forward(false);
    Xapian::MSet mymset3 = enquire.get_mset(0, 100);

    // mymset1 and mymset2 should be identical
    TEST_EQUAL(mymset1.size(), mymset2.size());

    {
	Xapian::MSetIterator i = mymset1.begin();
	Xapian::MSetIterator j = mymset2.begin();
	for ( ; i != mymset1.end(), j != mymset2.end(); ++i, j++) {
	    // if this fails, then setting match_sort_forward=true was not
	    // the same as the default.
	    TEST_EQUAL(*i, *j);
	}
    }

    // mymset1 and mymset3 should be same but reversed
    TEST_EQUAL(mymset1.size(), mymset3.size());

    {
	Xapian::MSetIterator i = mymset1.begin();
#ifdef __SUNPRO_CC
	vector<Xapian::docid> rev;
	for (Xapian::MSetIterator t = mymset3.begin(); t != mymset3.end(); ++t) 
	    rev.push_back(*t);
#else
	vector<Xapian::docid> rev(mymset3.begin(), mymset3.end());
#endif
	// Next iterator not const because of compiler brokenness (egcs 1.1.2)
	vector<Xapian::docid>::reverse_iterator j = rev.rbegin();
	for ( ; i != mymset1.end(); ++i, j++) {
	    // if this fails, then setting match_sort_forward=false didn't
	    // reverse the results.
	    TEST_EQUAL(*i, *j);
	}
    }

    return true;
}

// tests a reversed boolean query, where the full mset isn't returned
static bool test_reversebool2()
{
    Xapian::Enquire enquire(get_database("apitest_simpledata"));
    enquire.set_query(Xapian::Query("this"));
    enquire.set_weighting_scheme(Xapian::BoolWeight());

    Xapian::MSet mymset1 = enquire.get_mset(0, 100);

    TEST_AND_EXPLAIN(mymset1.size() > 1,
		     "Mset was too small to test properly");

    enquire.set_sort_forward(true);
    Xapian::doccount msize = mymset1.size() / 2;
    Xapian::MSet mymset2 = enquire.get_mset(0, msize);
    enquire.set_sort_forward(false);
    Xapian::MSet mymset3 = enquire.get_mset(0, msize);

    // mymset2 should be first msize items of mymset1
    TEST_EQUAL(msize, mymset2.size());

    {
	Xapian::MSetIterator i = mymset1.begin();
	Xapian::MSetIterator j = mymset2.begin();
	for ( ; i != mymset1.end(), j != mymset2.end(); ++i, j++) {
	    // if this fails, then setting match_sort_forward=true was not
	    // the same as the default.
	    TEST_EQUAL(*i, *j);
	}
    }

    // mymset3 should be last msize items of mymset1, in reverse order
    TEST_EQUAL(msize, mymset3.size());
    {
#ifdef __SUNPRO_CC
	vector<Xapian::docid> rev;
	for (Xapian::MSetIterator t = mymset1.begin(); t != mymset1.end(); ++t) 
	    rev.push_back(*t);
#else
	vector<Xapian::docid> rev(mymset1.begin(), mymset1.end());
#endif
	// Next iterator not const because of compiler brokenness (egcs 1.1.2)
	vector<Xapian::docid>::reverse_iterator i = rev.rbegin();
	Xapian::MSetIterator j = mymset3.begin();
	for ( ; j != mymset3.end(); ++i, j++) {
	    // if this fails, then setting match_sort_forward=false didn't
	    // reverse the results.
	    TEST_EQUAL(*i, *j);
	}
    }

    return true;
}

// tests that get_matching_terms() returns the terms in the right order
static bool test_getmterms1()
{
    om_termname_list answers_list;
    answers_list.push_back("one");
    answers_list.push_back("two");
    answers_list.push_back("three");
    answers_list.push_back("four");

    Xapian::Database mydb(get_database("apitest_termorder"));
    Xapian::Enquire enquire(mydb);

    Xapian::Query myquery(Xapian::Query::OP_OR,
	    Xapian::Query(Xapian::Query::OP_AND,
		    Xapian::Query("one", 1, 1),
		    Xapian::Query("three", 1, 3)),
	    Xapian::Query(Xapian::Query::OP_OR,
		    Xapian::Query("four", 1, 4),
		    Xapian::Query("two", 1, 2)));

    enquire.set_query(myquery);

    Xapian::MSet mymset = enquire.get_mset(0, 10);

    TEST_MSET_SIZE(mymset, 1);
#ifdef __SUNPRO_CC
    om_termname_list list;
    {
        Xapian::TermIterator t;
        for (t = enquire.get_matching_terms_begin(mymset.begin());
	     t != enquire.get_matching_terms_end(mymset.begin()); ++t) {
            list.push_back(*t);
	}
    }
#else
    om_termname_list list(enquire.get_matching_terms_begin(mymset.begin()),
			  enquire.get_matching_terms_end(mymset.begin()));
#endif
    TEST(list == answers_list);

    return true;
}

// tests that get_matching_terms() returns the terms only once
static bool test_getmterms2()
{
    om_termname_list answers_list;
    answers_list.push_back("one");
    answers_list.push_back("two");
    answers_list.push_back("three");

    Xapian::Database mydb(get_database("apitest_termorder"));
    Xapian::Enquire enquire(mydb);

    Xapian::Query myquery(Xapian::Query::OP_OR,
	    Xapian::Query(Xapian::Query::OP_AND,
		    Xapian::Query("one", 1, 1),
		    Xapian::Query("three", 1, 3)),
	    Xapian::Query(Xapian::Query::OP_OR,
		    Xapian::Query("one", 1, 4),
		    Xapian::Query("two", 1, 2)));

    enquire.set_query(myquery);

    Xapian::MSet mymset = enquire.get_mset(0, 10);

    TEST_MSET_SIZE(mymset, 1);
#ifdef __SUNPRO_CC
    om_termname_list list;
    {
        Xapian::TermIterator t;
        for (t = enquire.get_matching_terms_begin(mymset.begin());
	     t != enquire.get_matching_terms_end(mymset.begin()); ++t) {
            list.push_back(*t);
	}
    }
#else
    om_termname_list list(enquire.get_matching_terms_begin(mymset.begin()),
			  enquire.get_matching_terms_end(mymset.begin()));
#endif
    TEST(list == answers_list);

    return true;
}

// tests that the collapsing on termpos optimisation works
static bool test_poscollapse1()
{
    Xapian::Query myquery1(Xapian::Query::OP_OR,
		     Xapian::Query("this", 1, 1),
		     Xapian::Query("this", 1, 1));
    Xapian::Query myquery2("this", 2, 1);

    if (verbose) {
	tout << myquery1.get_description() << "\n";
	tout << myquery2.get_description() << "\n";
    }

    Xapian::MSet mymset1 = do_get_simple_query_mset(myquery1);
    Xapian::MSet mymset2 = do_get_simple_query_mset(myquery2);

    TEST_EQUAL(mymset1, mymset2);

    return true;
}

// tests that the collapsing on termpos optimisation gives correct query length
static bool test_poscollapse2()
{
    Xapian::Query q(Xapian::Query::OP_OR, Xapian::Query("this", 1, 1), Xapian::Query("this", 1, 1));
    TEST_EQUAL(q.get_length(), 2);
    return true;
}

// test that running a query twice returns the same results
static bool test_repeatquery1()
{
    Xapian::Enquire enquire(get_database("apitest_simpledata"));
    enquire.set_query(Xapian::Query("this"));

    enquire.set_query(query(Xapian::Query::OP_OR, "this", "word"));

    Xapian::MSet mymset1 = enquire.get_mset(0, 10);
    Xapian::MSet mymset2 = enquire.get_mset(0, 10);
    TEST_EQUAL(mymset1, mymset2);

    return true;
}

// test that prefetching documents works (at least, gives same results)
static bool test_fetchdocs1()
{
    Xapian::Enquire enquire(get_database("apitest_simpledata"));
    enquire.set_query(Xapian::Query("this"));

    enquire.set_query(query(Xapian::Query::OP_OR, "this", "word"));

    Xapian::MSet mymset1 = enquire.get_mset(0, 10);
    Xapian::MSet mymset2 = enquire.get_mset(0, 10);
    TEST_EQUAL(mymset1, mymset2);
    mymset2.fetch(mymset2[0], mymset2[mymset2.size() - 1]);
    mymset2.fetch(mymset2.begin(), mymset2.end());
    mymset2.fetch(mymset2.begin());
    mymset2.fetch();

    Xapian::MSetIterator it1 = mymset1.begin();
    Xapian::MSetIterator it2 = mymset2.begin();

    while (it1 != mymset1.end() && it2 != mymset2.end()) {
	TEST_EQUAL(it1.get_document().get_data(),
		   it2.get_document().get_data());
	TEST_NOT_EQUAL(it1.get_document().get_data(), "");
	TEST_NOT_EQUAL(it2.get_document().get_data(), "");
	it1++;
	it2++;
    }
    TEST_EQUAL(it1, mymset1.end());
    TEST_EQUAL(it1, mymset2.end());

    return true;
}

// test that searching for a term not in the database fails nicely
static bool test_absentterm1()
{
    Xapian::Enquire enquire(get_database("apitest_simpledata"));
    enquire.set_weighting_scheme(Xapian::BoolWeight());
    enquire.set_query(Xapian::Query("frink"));

    Xapian::MSet mymset = enquire.get_mset(0, 10);
    mset_expect_order(mymset);

    return true;
}

// as absentterm1, but setting query from a vector of terms
static bool test_absentterm2()
{
    Xapian::Enquire enquire(get_database("apitest_simpledata"));
    vector<string> terms;
    terms.push_back("frink");

    Xapian::Query query(Xapian::Query::OP_OR, terms.begin(), terms.end());
    enquire.set_query(query);

    Xapian::MSet mymset = enquire.get_mset(0, 10);
    mset_expect_order(mymset);

    return true;
}

// test that rsets do sensible things
static bool test_rset1()
{
    Xapian::Database mydb(get_database("apitest_rset"));
    Xapian::Enquire enquire(mydb);
    Xapian::Query myquery = query(Xapian::Query::OP_OR, "giraffe", "tiger");
    enquire.set_query(myquery);

    Xapian::MSet mymset1 = enquire.get_mset(0, 10);

    Xapian::RSet myrset;
    myrset.add_document(1);

    Xapian::MSet mymset2 = enquire.get_mset(0, 10, &myrset);

    // We should have the same documents turn up, but 1 and 3 should
    // have higher weights with the RSet.
    TEST_MSET_SIZE(mymset1, 3);
    TEST_MSET_SIZE(mymset2, 3);

    return true;
}

// test that rsets do more sensible things
static bool test_rset2()
{
    Xapian::Database mydb(get_database("apitest_rset"));
    Xapian::Enquire enquire(mydb);
    Xapian::Query myquery = query(Xapian::Query::OP_OR, "cuddly", "people");
    enquire.set_query(myquery);

    Xapian::MSet mymset1 = enquire.get_mset(0, 10);

    Xapian::RSet myrset;
    myrset.add_document(2);

    Xapian::MSet mymset2 = enquire.get_mset(0, 10, &myrset);

    mset_expect_order(mymset1, 1, 2);
    mset_expect_order(mymset2, 2, 1);

    return true;
}

// test that rsets behave correctly with multiDBs
static bool test_rsetmultidb1()
{
    Xapian::Database mydb1(get_database("apitest_rset", "apitest_simpledata2"));
    Xapian::Database mydb2(get_database("apitest_rset"));
    mydb2.add_database(get_database("apitest_simpledata2"));

    Xapian::Enquire enquire1(mydb1);
    Xapian::Enquire enquire2(mydb2);

    Xapian::Query myquery = query(Xapian::Query::OP_OR, "cuddly", "multiple");

    enquire1.set_query(myquery);
    enquire2.set_query(myquery);

    Xapian::RSet myrset1;
    Xapian::RSet myrset2;
    myrset1.add_document(4);
    myrset2.add_document(2);

    Xapian::MSet mymset1a = enquire1.get_mset(0, 10);
    Xapian::MSet mymset1b = enquire1.get_mset(0, 10, &myrset1);
    Xapian::MSet mymset2a = enquire2.get_mset(0, 10);
    Xapian::MSet mymset2b = enquire2.get_mset(0, 10, &myrset2);

    mset_expect_order(mymset1a, 1, 4);
    mset_expect_order(mymset1b, 4, 1);
    mset_expect_order(mymset2a, 1, 2);
    mset_expect_order(mymset2b, 2, 1);

    mset_range_is_same_weights(mymset1a, 0, mymset2a, 0, 2);
    mset_range_is_same_weights(mymset1b, 0, mymset2b, 0, 2);
    TEST_NOT_EQUAL(mymset1a, mymset1b);
    TEST_NOT_EQUAL(mymset2a, mymset2b);

    return true;
}

// regression tests - used to cause assertion in stats.h to fail
static bool test_rsetmultidb3()
{
    Xapian::Enquire enquire(get_database("apitest_simpledata2"));
    enquire.set_query(query(Xapian::Query::OP_OR, "cuddly", "people"));
    Xapian::MSet mset = enquire.get_mset(0, 10); // used to fail assertion
    return true;
}

/// Simple test of the elite set operator.
static bool test_eliteset1()
{
    Xapian::Database mydb(get_database("apitest_simpledata"));
    Xapian::Enquire enquire(mydb);

    Xapian::Query myquery1 = query(Xapian::Query::OP_OR, "word");
    myquery1.set_length(2); // so the query lengths are the same

    Xapian::Query myquery2 = query(Xapian::Query::OP_ELITE_SET, "simple", "word");
    myquery2.set_elite_set_size(1);

    enquire.set_query(myquery1);
    Xapian::MSet mymset1 = enquire.get_mset(0, 10);

    enquire.set_query(myquery2);
    Xapian::MSet mymset2 = enquire.get_mset(0, 10);

    TEST_EQUAL(mymset1, mymset2);
    return true;
}

/// Test that the elite set operator works if the set contains
/// sub-expressions (regression test)
static bool test_eliteset2()
{
    Xapian::Database mydb(get_database("apitest_simpledata"));
    Xapian::Enquire enquire(mydb);

    Xapian::Query myquery1 = query(Xapian::Query::OP_AND, "word", "search");

    Xapian::Query myquery2(Xapian::Query::OP_ELITE_SET,
		     query("this"),
		     query(Xapian::Query::OP_AND, "word", "search"));
    myquery2.set_elite_set_size(1);

    enquire.set_query(myquery1);
    Xapian::MSet mymset1 = enquire.get_mset(0, 10);

    enquire.set_query(myquery2);
    Xapian::MSet mymset2 = enquire.get_mset(0, 10);

    TEST_EQUAL(mymset1, mymset2);
    // query lengths differ so mset weights not the same (with some weighting
    // parameters)
    //test_mset_order_equal(mymset1, mymset2);

    return true;
}

/// Test that elite set doesn't affect query results if we have fewer
/// terms than the threshold
static bool test_eliteset3()
{
    Xapian::Database mydb1(get_database("apitest_simpledata"));
    Xapian::Enquire enquire1(mydb1);

    Xapian::Database mydb2(get_database("apitest_simpledata"));
    Xapian::Enquire enquire2(mydb2);

    // make a query
    Xapian::Stem stemmer("english");

    string term1 = stemmer.stem_word("word");
    string term2 = stemmer.stem_word("rubbish");
    string term3 = stemmer.stem_word("banana");

    vector<string> terms;
    terms.push_back(term1);
    terms.push_back(term2);
    terms.push_back(term3);

    Xapian::Query myquery1(Xapian::Query::OP_OR, terms.begin(), terms.end());
    enquire1.set_query(myquery1);

    Xapian::Query myquery2(Xapian::Query::OP_ELITE_SET, terms.begin(), terms.end());
    myquery2.set_elite_set_size(3);
    enquire2.set_query(myquery2);

    // retrieve the results
    Xapian::MSet mymset1 = enquire1.get_mset(0, 10);
    Xapian::MSet mymset2 = enquire2.get_mset(0, 10);

    TEST_EQUAL(mymset1.get_termfreq(term1),
	       mymset2.get_termfreq(term1));
    TEST_EQUAL(mymset1.get_termweight(term1),
	       mymset2.get_termweight(term1));
    TEST_EQUAL(mymset1.get_termfreq(term2),
	       mymset2.get_termfreq(term2));
    TEST_EQUAL(mymset1.get_termweight(term2),
	       mymset2.get_termweight(term2));
    TEST_EQUAL(mymset1.get_termfreq(term3),
	       mymset2.get_termfreq(term3));
    TEST_EQUAL(mymset1.get_termweight(term3),
	       mymset2.get_termweight(term3));
//    TEST_EQUAL(mymset1, mymset2);

    return true;
}

/// Test that elite set doesn't pick terms with 0 frequency
static bool test_eliteset4()
{
    Xapian::Database mydb1(get_database("apitest_simpledata"));
    Xapian::Enquire enquire1(mydb1);

    Xapian::Database mydb2(get_database("apitest_simpledata"));
    Xapian::Enquire enquire2(mydb2);

    Xapian::Query myquery1 = query("rubbish");
    Xapian::Query myquery2 = query(Xapian::Query::OP_ELITE_SET, "word", "rubbish", "fibble");
    myquery2.set_elite_set_size(1);
    enquire1.set_query(myquery1);
    enquire2.set_query(myquery2);

    // retrieve the results
    Xapian::MSet mymset1 = enquire1.get_mset(0, 10);
    Xapian::MSet mymset2 = enquire2.get_mset(0, 10);

    TEST_NOT_EQUAL(mymset2.size(), 0);
    TEST_EQUAL(mymset1, mymset2);
//    TEST_EQUAL(mymset1, mymset2);

    return true;
}

/// Test that the termfreq returned by termlists is correct.
static bool test_termlisttermfreq1()
{
    Xapian::Database mydb(get_database("apitest_simpledata"));
    Xapian::Enquire enquire(mydb);
    Xapian::Stem stemmer("english");
    Xapian::RSet rset1;
    Xapian::RSet rset2;
    rset1.add_document(5);
    rset2.add_document(6);

    Xapian::ESet eset1 = enquire.get_eset(1000, rset1);
    Xapian::ESet eset2 = enquire.get_eset(1000, rset2);

    // search for weight of term 'another'
    string theterm = stemmer.stem_word("another");

    Xapian::weight wt1 = 0;
    Xapian::weight wt2 = 0;
    {
	Xapian::ESetIterator i = eset1.begin();
	for ( ; i != eset1.end(); i++) {
	    if (*i == theterm) {
		wt1 = i.get_weight();
		break;
	    }
	}
    }
    {
	Xapian::ESetIterator i = eset2.begin();
	for ( ; i != eset2.end(); i++) {
	    if (*i == theterm) {
		wt2 = i.get_weight();
		break;
	    }
	}
    }

    TEST_NOT_EQUAL(wt1, 0);
    TEST_NOT_EQUAL(wt2, 0);
    TEST_EQUAL(wt1, wt2);

    return true;
}

/// Test the termfrequency and termweight info returned for query terms
static bool test_qterminfo1()
{
    Xapian::Database mydb1(get_database("apitest_simpledata", "apitest_simpledata2"));
    Xapian::Enquire enquire1(mydb1);

    Xapian::Database mydb2(get_database("apitest_simpledata"));
    mydb2.add_database(get_database("apitest_simpledata2"));
    Xapian::Enquire enquire2(mydb2);

    // make a query
    Xapian::Stem stemmer("english");

    string term1 = stemmer.stem_word("word");
    string term2 = stemmer.stem_word("inmemory");
    string term3 = stemmer.stem_word("flibble");

    Xapian::Query myquery(Xapian::Query::OP_OR,
		    Xapian::Query(term1),
		    Xapian::Query(Xapian::Query::OP_OR,
			    Xapian::Query(term2),
			    Xapian::Query(term3)));
    enquire1.set_query(myquery);
    enquire2.set_query(myquery);

    // retrieve the results
    Xapian::MSet mymset1a = enquire1.get_mset(0, 0);
    Xapian::MSet mymset2a = enquire2.get_mset(0, 0);

    TEST_EQUAL(mymset1a.get_termfreq(term1),
	       mymset2a.get_termfreq(term1));
    TEST_EQUAL(mymset1a.get_termweight(term1),
	       mymset2a.get_termweight(term1));
    TEST_EQUAL(mymset1a.get_termfreq(term2),
	       mymset2a.get_termfreq(term2));
    TEST_EQUAL(mymset1a.get_termweight(term2),
	       mymset2a.get_termweight(term2));
    TEST_EQUAL(mymset1a.get_termfreq(term3),
	       mymset2a.get_termfreq(term3));
    TEST_EQUAL(mymset1a.get_termweight(term3),
	       mymset2a.get_termweight(term3));

    TEST_EQUAL(mymset1a.get_termfreq(term1), 3);
    TEST_EQUAL(mymset1a.get_termfreq(term2), 1);
    TEST_EQUAL(mymset1a.get_termfreq(term3), 0);

    TEST_NOT_EQUAL(mymset1a.get_termweight(term1), 0);
    TEST_NOT_EQUAL(mymset1a.get_termweight(term2), 0);
    // non-existant terms still have weight
    TEST_NOT_EQUAL(mymset1a.get_termweight(term3), 0);

    TEST_EXCEPTION(Xapian::InvalidArgumentError,
		   mymset1a.get_termfreq("sponge"));

    return true;
}

/// Regression test for bug #37.
static bool test_qterminfo2()
{
    Xapian::Database db(get_database("apitest_simpledata"));
    Xapian::Enquire enquire(db);

    // make a query
    Xapian::Stem stemmer("english");

    string term1 = stemmer.stem_word("paragraph");
    string term2 = stemmer.stem_word("another");

    Xapian::Query query(Xapian::Query::OP_AND_NOT, term1,
	    Xapian::Query(Xapian::Query::OP_AND, term1, term2));
    enquire.set_query(query);

    // retrieve the results
    // Note: get_mset() used to throw "AssertionError" in debug builds
    Xapian::MSet mset = enquire.get_mset(0, 10);

    TEST_NOT_EQUAL(mset.get_termweight("paragraph"), 0);

    return true;
}

// tests that when specifiying that no items are to be returned, those
// statistics which should be the same are.
static bool test_msetzeroitems1()
{
    Xapian::MSet mymset1 = do_get_simple_query_mset(query("this"), 0);
    Xapian::MSet mymset2 = do_get_simple_query_mset(query("this"), 1);

    TEST_EQUAL(mymset1.get_max_possible(), mymset2.get_max_possible());

    return true;
}

// test that the matches_* of a simple query are as expected
static bool test_matches1()
{
    Xapian::Query myquery;
    Xapian::MSet mymset;

    myquery = query("word");
    mymset = do_get_simple_query_mset(myquery);
    TEST_EQUAL(mymset.get_matches_lower_bound(), 2);
    TEST_EQUAL(mymset.get_matches_estimated(), 2);
    TEST_EQUAL(mymset.get_matches_upper_bound(), 2);

    myquery = query(Xapian::Query::OP_OR, "inmemory", "word");
    mymset = do_get_simple_query_mset(myquery);
    TEST_EQUAL(mymset.get_matches_lower_bound(), 2);
    TEST_EQUAL(mymset.get_matches_estimated(), 2);
    TEST_EQUAL(mymset.get_matches_upper_bound(), 2);

    myquery = query(Xapian::Query::OP_AND, "inmemory", "word");
    mymset = do_get_simple_query_mset(myquery);
    TEST_EQUAL(mymset.get_matches_lower_bound(), 0);
    TEST_EQUAL(mymset.get_matches_estimated(), 0);
    TEST_EQUAL(mymset.get_matches_upper_bound(), 0);

    myquery = query(Xapian::Query::OP_AND, "simple", "word");
    mymset = do_get_simple_query_mset(myquery);
    TEST_EQUAL(mymset.get_matches_lower_bound(), 2);
    TEST_EQUAL(mymset.get_matches_estimated(), 2);
    TEST_EQUAL(mymset.get_matches_upper_bound(), 2);

    myquery = query(Xapian::Query::OP_AND, "simple", "word");
    mymset = do_get_simple_query_mset(myquery, 0);
    TEST_EQUAL(mymset.get_matches_lower_bound(), 0);
    TEST_EQUAL(mymset.get_matches_estimated(), 1);
    TEST_EQUAL(mymset.get_matches_upper_bound(), 2);

    mymset = do_get_simple_query_mset(myquery, 1);
    TEST_EQUAL(mymset.get_matches_lower_bound(), 2);
    TEST_EQUAL(mymset.get_matches_estimated(), 2);
    TEST_EQUAL(mymset.get_matches_upper_bound(), 2);

    mymset = do_get_simple_query_mset(myquery, 2);
    TEST_EQUAL(mymset.get_matches_lower_bound(), 2);
    TEST_EQUAL(mymset.get_matches_estimated(), 2);
    TEST_EQUAL(mymset.get_matches_upper_bound(), 2);

    myquery = query(Xapian::Query::OP_AND, "paragraph", "another");
    mymset = do_get_simple_query_mset(myquery, 0);
    TEST_EQUAL(mymset.get_matches_lower_bound(), 0);
    TEST_EQUAL(mymset.get_matches_estimated(), 1);
    TEST_EQUAL(mymset.get_matches_upper_bound(), 2);

    mymset = do_get_simple_query_mset(myquery, 1);
    TEST_EQUAL(mymset.get_matches_lower_bound(), 1);
    TEST_EQUAL(mymset.get_matches_estimated(), 1);
    TEST_EQUAL(mymset.get_matches_upper_bound(), 2);

    mymset = do_get_simple_query_mset(myquery, 2);
    TEST_EQUAL(mymset.get_matches_lower_bound(), 1);
    TEST_EQUAL(mymset.get_matches_estimated(), 1);
    TEST_EQUAL(mymset.get_matches_upper_bound(), 1);

    mymset = do_get_simple_query_mset(myquery, 1, 20);
    TEST_EQUAL(mymset.get_matches_lower_bound(), 1);
    TEST_EQUAL(mymset.get_matches_estimated(), 1);
    TEST_EQUAL(mymset.get_matches_upper_bound(), 1);

    return true;
}

// tests that wqf affects the document weights
static bool test_wqf1()
{
    // both queries have length 2; in q1 word has wqf=2, in q2 word has wqf=1
    Xapian::Query q1("word", 2);
    Xapian::Query q2("word");
    q2.set_length(2);
    Xapian::MSet mset1 = do_get_simple_query_mset(q1);
    Xapian::MSet mset2 = do_get_simple_query_mset(q2);
    // Check the weights
    TEST(mset1.begin().get_weight() > mset2.begin().get_weight());
    return true;
}

// tests that query length affects the document weights
static bool test_qlen1()
{
    Xapian::Query q1("word");
    Xapian::Query q2("word");
    q2.set_length(2);
    Xapian::MSet mset1 = do_get_simple_query_mset(q1);
    Xapian::MSet mset2 = do_get_simple_query_mset(q2);
    // Check the weights
    //TEST(mset1.begin().get_weight() < mset2.begin().get_weight());
    TEST(mset1.begin().get_weight() == mset2.begin().get_weight());
    return true;
}

// tests that opening a non-existant termlist throws the correct exception
static bool test_termlist1()
{
    Xapian::Database db(get_database("apitest_onedoc"));
    TEST_EXCEPTION(Xapian::InvalidArgumentError,
		   Xapian::TermIterator t = db.termlist_begin(0));
    TEST_EXCEPTION(Xapian::DocNotFoundError,
		   Xapian::TermIterator t = db.termlist_begin(2));
    /* Cause the database to be used properly, showing up problems
     * with the link being in a bad state.  CME */
    Xapian::TermIterator temp = db.termlist_begin(1);
    TEST_EXCEPTION(Xapian::DocNotFoundError,
		   Xapian::TermIterator t = db.termlist_begin(999999999));
    return true;
}

// tests that a Xapian::TermIterator works as an STL iterator
static bool test_termlist2()
{
    Xapian::Database db(get_database("apitest_onedoc"));
    Xapian::TermIterator t = db.termlist_begin(1);
    Xapian::TermIterator tend = db.termlist_end(1);

    // test operator= creates a copy which compares equal
    Xapian::TermIterator t_copy = t;
    TEST_EQUAL(t, t_copy);

    // test copy constructor creates a copy which compares equal
    Xapian::TermIterator t_clone(t);
    TEST_EQUAL(t, t_clone);

#ifdef __SUNPRO_CC
    vector<string> v;
    while (t != tend) {
	v.push_back(*t);
	++t;
    }
#else
    vector<string> v(t, tend);
#endif

    t = db.termlist_begin(1);    
    tend = db.termlist_end(1);
    vector<string>::const_iterator i;
    for (i = v.begin(); i != v.end(); i++) {
	TEST_NOT_EQUAL(t, tend);
	TEST_EQUAL(*i, *t);
	t++;
    }
    TEST_EQUAL(t, tend);
    return true;
}

static Xapian::TermIterator
test_termlist3_helper()
{
    Xapian::Database db(get_database("apitest_onedoc"));
    return db.termlist_begin(1);
}

// tests that a Xapian::TermIterator still works when the DB is deleted
static bool test_termlist3()
{
    Xapian::TermIterator u = test_termlist3_helper();
    Xapian::Database db(get_database("apitest_onedoc"));
    Xapian::TermIterator t = db.termlist_begin(1);
    Xapian::TermIterator tend = db.termlist_end(1);

    while (t != tend) {
	TEST_EQUAL(*t, *u);
	t++;
	u++;
    }
    return true;
}

// tests skip_to
static bool test_termlist4()
{
    Xapian::Database db(get_database("apitest_onedoc"));
    Xapian::TermIterator i = db.termlist_begin(1);
    i.skip_to("");
    i.skip_to("\xff");
    return true;
}

// tests punctuation is OK in terms (particularly in remote queries)
static bool test_puncterms1()
{
    Xapian::Database db(get_database("apitest_punc"));
    Xapian::Enquire enquire(db);

    Xapian::Query q1("semi;colon");
    enquire.set_query(q1);
    Xapian::MSet m1 = enquire.get_mset(0, 10);

    Xapian::Query q2("col:on");
    enquire.set_query(q2);
    Xapian::MSet m2 = enquire.get_mset(0, 10);

    Xapian::Query q3("com,ma");
    enquire.set_query(q3);
    Xapian::MSet m3 = enquire.get_mset(0, 10);

    return true;
}

// test that searching for a term with a space or backslash in it works
static bool test_spaceterms1()
{
    Xapian::Enquire enquire(get_database("apitest_space"));
    Xapian::MSet mymset;
    Xapian::doccount count;
    Xapian::MSetIterator m;
    Xapian::Stem stemmer("english");

    enquire.set_query(stemmer.stem_word("space man"));
    mymset = enquire.get_mset(0, 10);
    TEST_MSET_SIZE(mymset, 1);
    count = 0;
    for (m = mymset.begin(); m != mymset.end(); ++m) ++count;
    TEST_EQUAL(count, 1);

    for (Xapian::valueno value_no = 1; value_no < 7; ++value_no) {
	TEST_NOT_EQUAL(mymset.begin().get_document().get_data(), "");
	TEST_NOT_EQUAL(mymset.begin().get_document().get_value(value_no), "");
    }

    enquire.set_query(stemmer.stem_word("tab\tby"));
    mymset = enquire.get_mset(0, 10);
    TEST_MSET_SIZE(mymset, 1);
    count = 0;
    for (m = mymset.begin(); m != mymset.end(); ++m) ++count;
    TEST_EQUAL(count, 1);

    for (Xapian::valueno value_no = 1; value_no < 7; ++value_no) {
	string value = mymset.begin().get_document().get_value(value_no);
	TEST_NOT_EQUAL(value, "");
	if (value_no == 0) {
	    TEST(value.size() > 262);
	    TEST_EQUAL(static_cast<unsigned char>(value[261]), 255);
	}
    }
    
    enquire.set_query(stemmer.stem_word("back\\slash"));
    mymset = enquire.get_mset(0, 10);
    TEST_MSET_SIZE(mymset, 1);
    count = 0;
    for (m = mymset.begin(); m != mymset.end(); ++m) ++count;
    TEST_EQUAL(count, 1);

    return true;
}

// test that XOR queries work
static bool test_xor1()
{
    Xapian::Enquire enquire(get_database("apitest_simpledata"));
    enquire.set_query(Xapian::Query("this"));
    Xapian::Stem stemmer("english");

    vector<string> terms;
    terms.push_back(stemmer.stem_word("this"));
    terms.push_back(stemmer.stem_word("word"));
    terms.push_back(stemmer.stem_word("of"));

    Xapian::Query query(Xapian::Query::OP_XOR, terms.begin(), terms.end());
    enquire.set_weighting_scheme(Xapian::BoolWeight());
    enquire.set_query(query);

    Xapian::MSet mymset = enquire.get_mset(0, 10);
    mset_expect_order(mymset, 1, 2, 5, 6);

    return true;
}

// test Xapian::Database::get_document()
static bool test_getdoc1()
{
    Xapian::Database db(get_database("apitest_onedoc"));
    Xapian::Document doc(db.get_document(1));
    TEST_EXCEPTION(Xapian::InvalidArgumentError, db.get_document(0));
    TEST_EXCEPTION(Xapian::DocNotFoundError, db.get_document(999999999));    
    TEST_EXCEPTION(Xapian::DocNotFoundError, db.get_document(123456789));
    TEST_EXCEPTION(Xapian::DocNotFoundError, db.get_document(3));
    TEST_EXCEPTION(Xapian::DocNotFoundError, db.get_document(2));
    // Check that Document works as a handle on modification
    // (this was broken for the first try at Xapian::Document prior to 0.7).
    Xapian::Document doc2 = doc;
    doc.set_data("modified!");
    TEST_EQUAL(doc.get_data(), "modified!");
    TEST_EQUAL(doc.get_data(), doc2.get_data());
    return true;
}

// test whether operators with no elements work as a null query
static bool test_emptyop1()
{
    Xapian::Enquire enquire(get_database("apitest_simpledata"));
    vector<Xapian::Query> nullvec;
    
    Xapian::Query query1(Xapian::Query::OP_XOR, nullvec.begin(), nullvec.end());

    Xapian::MSet mymset = do_get_simple_query_mset(query1);
    TEST_MSET_SIZE(mymset, 0);
    TEST_EXCEPTION(Xapian::InvalidArgumentError, enquire.get_matching_terms_begin(1));

    return true;
}

// Regression test for check_at_least SEGV when there are no matches.
static bool test_checkatleast1()
{
    Xapian::Enquire enquire(get_database("apitest_simpledata"));
    enquire.set_query(Xapian::Query("thom"));
    Xapian::MSet mymset = enquire.get_mset(0, 10, 11);
    TEST_EQUAL(0, mymset.size());

    return true;
}

// #######################################################################
// # End of test cases: now we list the tests to run.

/// The tests which work with any backend
test_desc anydb_tests[] = {
    {"zerodocid1", 	   test_zerodocid1},
    {"emptyquery1",	   test_emptyquery1},
    {"simplequery1",       test_simplequery1},
    {"simplequery2",       test_simplequery2},
    {"simplequery3",       test_simplequery3},
    {"multidb1",           test_multidb1},
    {"multidb2",           test_multidb2},
    {"multidb3",           test_multidb3},
    {"multidb4",           test_multidb4},
    {"multidb5",           test_multidb5},
    {"changequery1",	   test_changequery1},
    {"msetmaxitems1",      test_msetmaxitems1},
    {"expandmaxitems1",    test_expandmaxitems1},
    {"boolquery1",         test_boolquery1},
    {"msetfirst1",         test_msetfirst1},
    {"msetfirst2",         test_msetfirst2},
    {"topercent1",	   test_topercent1},
    {"expandfunctor1",	   test_expandfunctor1},
    {"pctcutoff1",	   test_pctcutoff1},
    {"cutoff1",		   test_cutoff1},
    {"cutoff2",		   test_cutoff2},
    {"allowqterms1",       test_allowqterms1},
    {"maxattain1",         test_maxattain1},
    {"reversebool1",	   test_reversebool1},
    {"reversebool2",	   test_reversebool2},
    {"getmterms1",	   test_getmterms1},
    {"getmterms2",	   test_getmterms2},
    {"poscollapse1",	   test_poscollapse1},
    {"poscollapse2",	   test_poscollapse2},
    {"repeatquery1",	   test_repeatquery1},
    {"fetchdocs1",	   test_fetchdocs1},
    {"absentterm1",	   test_absentterm1},
    {"absentterm2",	   test_absentterm2},
    {"rset1",              test_rset1},
    {"rset2",              test_rset2},
    {"rsetmultidb1",       test_rsetmultidb1},
    {"rsetmultidb3",       test_rsetmultidb3},
    {"eliteset1",          test_eliteset1},
    {"eliteset2",          test_eliteset2},
    {"eliteset3",          test_eliteset3},
    {"eliteset4",          test_eliteset4},
    {"termlisttermfreq1",  test_termlisttermfreq1},
    {"qterminfo1",	   test_qterminfo1},
    {"qterminfo2",	   test_qterminfo2},
    {"msetzeroitems1",     test_msetzeroitems1},
    {"matches1",	   test_matches1},
    {"wqf1",		   test_wqf1},
    {"qlen1",		   test_qlen1},
    {"termlist1",	   test_termlist1},
    {"termlist2",	   test_termlist2},
    {"termlist3",	   test_termlist3},
    {"termlist4",	   test_termlist4},
    {"puncterms1",	   test_puncterms1},
    {"spaceterms1",	   test_spaceterms1},
    {"xor1",		   test_xor1},
    {"getdoc1",		   test_getdoc1},
    {"emptyop1",	   test_emptyop1},
    {"checkatleast1",	   test_checkatleast1},
    {0, 0}
};
