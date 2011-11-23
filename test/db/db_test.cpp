#include <string>

#include <gtest/gtest.h>

#include "object_recognition/common/json_spirit/json_spirit.h"
#include <object_recognition/db/db.h>

const char* db_url = "http://localhost:5984";
using namespace object_recognition::db;

using object_recognition::db::ObjectDbParameters;

ObjectDbParameters
params_bogus(const std::string& url = db_url)
{
  std::map<std::string, std::string> params;
  params["urld"] = url;
  return ObjectDbParameters(params);
}

std::string
params_garbage(const std::string& url = db_url)
{
  return "{\ndfkja:dkfj, dfkjak, dfkjalksf.dfj ---\ndfjkasdf";
}

ObjectDbParameters
params_test()
{
  ObjectDbParameters params = ObjectDbParameters("CouchDB");
  params.root_ = "http://foo:12323";
  params.collection_ = "test_it";
  return params;
}

ObjectDbParameters
params_valid()
{
  ObjectDbParameters params = ObjectDbParameters("CouchDB");
  params.root_ = "http://localhost:5984";
  params.collection_ = "test_it";
  return params;
}

or_json::mObject
parse_status(const std::string& status)
{
  or_json::mValue value;
  std::stringstream ss(status);
  or_json::read(ss, value);
  return value.get_obj();
}

void
expect_not_found(ObjectDb& db, const std::string& collection)
{
  std::string status;
  db.Status(collection, status);
  or_json::mObject ps = parse_status(status);
  EXPECT_EQ(ps["error"].get_str(), "not_found");
}

void
delete_c(ObjectDb& db, const std::string& collection)
{
  db.DeleteCollection(collection);
  expect_not_found(db, collection);
}

TEST(OR_db, Status)
{
  ObjectDb db(ObjectDbParameters("CouchDB"));
  std::string status;
  db.Status(status);
  or_json::mObject ps = parse_status(status);
  EXPECT_EQ(ps.count("couchdb"), 1);
}

TEST(OR_db, CreateDelete)
{
  {
    ObjectDb db(ObjectDbParameters("CouchDB"));
    db.CreateCollection("test_it");
    std::string status;
    db.Status("test_it", status);
    or_json::mObject ps = parse_status(status);
    EXPECT_EQ(ps["db_name"].get_str(), "test_it");
  }
  {
    ObjectDb db(ObjectDbParameters("CouchDB"));
    delete_c(db, "test_it");
  }
}
TEST(OR_db, DeleteNonexistant)
{
  ObjectDb db(ObjectDbParameters("CouchDB"));
  db.DeleteCollection("dgadf");
}

TEST(OR_db, DocumentPesistLoad)
{
  ObjectDb db(params_valid());
  delete_c(db, "test_it");
  std::string id;
  {
    Document doc(db);
    doc.set_value("x", 1.0);
    doc.set_value("foo", "UuU");
    doc.Persist();
    id = doc.id();
  }
  {
    Document doc(db, id);
    EXPECT_EQ(doc.get_value<double>("x"), 1.0);
    EXPECT_EQ(doc.get_value<std::string>("foo"), "UuU");
  }
  delete_c(db, "test_it");
}

TEST(OR_db, NonExistantCouch)
{
  ObjectDbParameters params = params_test();
  ObjectDb db(params);
  try
  {
    std::string status;
    db.Status(status);
    ASSERT_FALSE(true);
  } catch (std::runtime_error& e)
  {
    EXPECT_EQ(std::string(e.what()), "No response from server. : http://foo:12323");
  }

}

TEST(OR_db, StatusCollectionNonExistantDb)
{
  ObjectDbParameters params = params_test();
  ObjectDb db(params);
  try
  {
    std::string status;
    db.Status("test_it", status);
    ASSERT_FALSE(true);
  } catch (std::runtime_error& e)
  {
    EXPECT_EQ(std::string(e.what()), "No response from server. : http://foo:12323/test_it");
  }
}

TEST(OR_db, DeleteBogus)
{
  ObjectDbParameters params = params_test();
  ObjectDb db(params);
  try
  {
    db.DeleteCollection("test_it");
    ASSERT_FALSE(true);
  } catch (std::runtime_error& e)
  {
    EXPECT_EQ(std::string(e.what()), "No response from server. : http://foo:12323/test_it");
  }
}
TEST(OR_db, StatusCollectionNonExistant)
{
  ObjectDb db(ObjectDbParameters("CouchDB"));
  db.DeleteCollection("test_it");

  std::string status;
  db.Status("test_it", status);
  or_json::mObject ps = parse_status(status);
  EXPECT_EQ(ps["error"].get_str(), "not_found");
  EXPECT_EQ(ps["reason"].get_str(), "no_db_file");
}

TEST(OR_db, StatusCollectionExistant)
{
  ObjectDb db(ObjectDbParameters("CouchDB"));
  db.DeleteCollection("test_it");
  std::string status;
  db.CreateCollection("test_it");
  db.Status("test_it", status);
  or_json::mObject ps = parse_status(status);
  EXPECT_EQ(ps["db_name"].get_str(), "test_it");
  db.DeleteCollection("test_it");
}

TEST(OR_db, DocumentBadId)
{
  ObjectDb db(params_valid());
  try
  {
    Document doc(db, "bogus_id");
    ASSERT_FALSE(true);

  } catch (std::runtime_error& e)
  {
    EXPECT_EQ(std::string(e.what()), "Object Not Found : http://localhost:5984/test_it/bogus_id");
  }
}

TEST(OR_db, DocumentUrl)
{
  ObjectDbParameters params = params_test();
  params.collection_ = "test_it";
  ObjectDb db(params);
  try
  {
    Document doc(db, "bogus_id");
    ASSERT_FALSE(true);
  } catch (std::runtime_error& e)
  {
    EXPECT_EQ(std::string(e.what()), "No response from server. : http://foo:12323/test_it/bogus_id");
  }
}

TEST(OR_db, DoubleCreate)
{
  ObjectDb db(ObjectDbParameters("CouchDB"));
  db.CreateCollection("aa");
  db.CreateCollection("aa");
  db.DeleteCollection("aa");
}

TEST(OR_db, DoubleDelete)
{
  ObjectDb db(ObjectDbParameters("CouchDB"));
  db.CreateCollection("aa");
  db.DeleteCollection("aa");
  db.DeleteCollection("aa");
}

TEST(OR_db, ParamsBogus)
{
  try
  {
    ObjectDb db(params_bogus());
    ASSERT_FALSE(true);
  } catch (std::runtime_error& e)
  {
    EXPECT_EQ(std::string("You must supply a database type. e.g. CouchDB"), e.what());
  }
}

TEST(OR_db, ParamsGarbage)
{
  try
  {
    ObjectDb db(params_garbage());
    ASSERT_FALSE(true);
  } catch (std::runtime_error& e)
  {
    std::string error(e.what());
    EXPECT_TRUE(error.find(std::string("Invalid type.")) != std::string::npos);
  }
}

TEST(OR_db, NonArgsDbCreate)
{
  ObjectDb db;
  try
  {
    db.CreateCollection("aa");
    ASSERT_FALSE(true);
  } catch (std::runtime_error& e)
  {
    std::string expect = "This ObjectDb instance is uninitialized.";
    EXPECT_EQ(e.what(), expect);
  }
}
TEST(OR_db, NonArgsDbInsert)
{
  ObjectDb db;

  std::string id;
  {
    Document doc(db);
    doc.set_value("x", 1.0);
    doc.set_value("foo", "UuU");
    try
    {
      doc.Persist();
      ASSERT_FALSE(true);
    } catch (std::runtime_error& e)
    {
      std::string expect = "This ObjectDb instance is uninitialized.";
      EXPECT_EQ(e.what(), expect);
    }
  }
}

TEST(OR_db, InitSeperatelyChangeURL)
{
  ObjectDb db;
  db.set_parameters(ObjectDbParameters("CouchDB"));
  std::string status;
  db.Status(status);
  or_json::mObject ps = parse_status(status);
  EXPECT_EQ(ps.count("couchdb"), 1);
  ObjectDbParameters params("CouchDB");
  params.root_ = "http://abc";
  db.set_parameters(params);
  try
  {
    db.Status(status);
    ASSERT_FALSE(true);
  } catch (std::runtime_error& e)
  {
//error messages seem to be bit platform dependent.
//    std::string expect = "No response from server. : http://abc";
//    EXPECT_EQ(e.what(), expect);
  }
}

TEST(OR_db, ObjectDbCopy)
{
  ObjectDb db(ObjectDbParameters("CouchDB")), db2;
  db2 = db;
  std::string s1, s2;
  db.Status(s1);
  db2.Status(s2);
  EXPECT_EQ(s1, s2);
}

TEST(OR_db, JSONReadWrite)
{
  or_json::mObject params1, params2;
  std::stringstream ssparams1, ssparams2, ssparams3;
  ssparams1 << "{\"num1\":2, \"num2\":3.5, \"str\":\"foo\"}";
  or_json::mValue value;
  or_json::read(ssparams1, value);
  params1 = value.get_obj();

  // Write it to a JSON string and make sure numbers are persisted as numbers
  value = or_json::mValue(params1);
  or_json::write(value, ssparams2);
  std::string new_json = ssparams2.str();
  EXPECT_GE(new_json.find("\"2\""), new_json.size());
  EXPECT_GE(new_json.find("\"3.5\""), new_json.size());
  EXPECT_LT(new_json.find("\"foo\""), new_json.size());

  // Make sure we can read it back
  ssparams3 << ssparams2.str();
  or_json::read(ssparams3, value);
  params2 = value.get_obj();
  EXPECT_EQ(params1["num1"].get_uint64(), params2["num1"].get_uint64());
  EXPECT_EQ(params1["num2"].get_real(), params2["num2"].get_real());
  EXPECT_EQ(params1["str"].get_str(), params2["str"].get_str());
}

TEST(OR_db, JSONReadBigInteger)
{
  or_json::mObject params1, params2;
  std::stringstream ssparams1, ssparams2, ssparams3;
  ssparams1 << "{\"num\":3372036854775808  }";
  or_json::mValue value;
  or_json::read(ssparams1, value);
  params1 = value.get_obj();
}
