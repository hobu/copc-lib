#include "data/test_data.h"
#include <catch2/catch.hpp>
#include <copc-lib/io/reader.hpp>
#include <copc-lib/io/writer.hpp>
#include <sstream>

using namespace copc;
using namespace std;

TEST_CASE("Writer Node Uncompressed", "[Writer]")
{
    SECTION("Add one")
    {
        stringstream out_stream;

        Writer::LasConfig cfg(3);
        Writer writer(out_stream, cfg);

        Page root_page = writer.GetRootPage();

        REQUIRE_THROWS(writer.AddNode(root_page, VoxelKey::InvalidKey(), std::vector<char>()));
        REQUIRE_THROWS(writer.AddNode(root_page, VoxelKey(0, 0, 0, 0), std::vector<char>()));

        std::vector<char> root_node(twenty_pts_prdf3, twenty_pts_prdf3 + sizeof(twenty_pts_prdf3));
        REQUIRE_NOTHROW(writer.AddNode(root_page, VoxelKey(0, 0, 0, 0), root_node));

        writer.Close();

        Reader reader(out_stream);
        REQUIRE(reader.GetCopcHeader().root_hier_offset > 0);
        REQUIRE(reader.GetCopcHeader().root_hier_size == 32);

        auto node = reader.FindNode(VoxelKey::BaseKey());
        REQUIRE(node.IsValid());
        auto root_node_test = reader.GetPointData(node);

        REQUIRE(root_node_test == root_node);
    }

    SECTION("Add multiple")
    {
        stringstream out_stream;

        Writer::LasConfig cfg(3);
        Writer writer(out_stream, cfg, 256, "test_wkt");

        Page root_page = writer.GetRootPage();

        std::vector<char> twenty(twenty_pts_prdf3, twenty_pts_prdf3 + sizeof(twenty_pts_prdf3));
        REQUIRE_NOTHROW(writer.AddNode(root_page, VoxelKey(0, 0, 0, 0), twenty));

        std::vector<char> twelve(twelve_pts_prdf3, twelve_pts_prdf3 + sizeof(twelve_pts_prdf3));
        REQUIRE_NOTHROW(writer.AddNode(root_page, VoxelKey(1, 1, 1, 1), twelve));

        std::vector<char> sixty(sixty_pts_prdf3, sixty_pts_prdf3 + sizeof(sixty_pts_prdf3));
        REQUIRE_NOTHROW(writer.AddNode(root_page, VoxelKey(1, 1, 1, 0), sixty));

        writer.Close();

        std::string ostr = out_stream.str();
        Reader reader(out_stream);
        REQUIRE(reader.GetCopcHeader().root_hier_offset > 0);
        REQUIRE(reader.GetCopcHeader().root_hier_size == 32 * 3);

        {
            auto node = reader.FindNode(VoxelKey::BaseKey());
            REQUIRE(node.IsValid());
            auto node_data = reader.GetPointData(node);
            REQUIRE(node_data == twenty);
        }

        {
            auto node = reader.FindNode(VoxelKey(1, 1, 1, 1));
            REQUIRE(node.IsValid());
            auto node_data = reader.GetPointData(node);
            REQUIRE(node_data == twelve);
        }

        {
            auto node = reader.FindNode(VoxelKey(1, 1, 1, 0));
            REQUIRE(node.IsValid());
            auto node_data = reader.GetPointData(node);
            REQUIRE(node_data == sixty);
        }
    }

    SECTION("Add hierarchy")
    {
        stringstream out_stream;

        Writer::LasConfig cfg(3);
        Writer writer(out_stream, cfg);

        Page root_page = writer.GetRootPage();
        Page sub_page1 = writer.AddSubPage(root_page, VoxelKey(1, 0, 0, 0));
        Page sub_page2 = writer.AddSubPage(root_page, VoxelKey(1, 1, 1, 1));

        std::vector<char> twenty(twenty_pts_prdf3, twenty_pts_prdf3 + sizeof(twenty_pts_prdf3));
        REQUIRE_NOTHROW(writer.AddNode(root_page, VoxelKey(0, 0, 0, 0), twenty));

        std::vector<char> twelve(twelve_pts_prdf3, twelve_pts_prdf3 + sizeof(twelve_pts_prdf3));
        REQUIRE_NOTHROW(writer.AddNode(sub_page1, VoxelKey(1, 0, 0, 0), twelve));

        std::vector<char> sixty(sixty_pts_prdf3, sixty_pts_prdf3 + sizeof(sixty_pts_prdf3));
        REQUIRE_NOTHROW(writer.AddNode(sub_page2, VoxelKey(1, 1, 1, 1), sixty));
        REQUIRE_NOTHROW(writer.AddNode(sub_page2, VoxelKey(2, 2, 2, 2), twenty));

        // Can't add a node that's not a child of the page
        REQUIRE_THROWS(writer.AddNode(sub_page2, VoxelKey(1, 2, 2, 2), twenty));

        writer.Close();

        /*
            hierarchy should look like this:
            Page 0-0-0-0
                Node 0-0-0-0
                Page 1-0-0-0
                    Node 1-0-0-0
                Page 1-1-1-1
                    Node 1-1-1-1
                    Node 2-2-2-2
        */

        Reader reader(out_stream);
        REQUIRE(reader.GetCopcHeader().root_hier_offset > 0);
        REQUIRE(reader.GetCopcHeader().root_hier_size == 32 * 3);

        {
            auto sub_node = reader.FindNode(VoxelKey(2, 2, 2, 2));
            REQUIRE(sub_node.IsValid());
            auto sub_node_data = reader.GetPointData(sub_node);
            REQUIRE(sub_node_data == twenty);
        }
        {
            auto sub_node = reader.FindNode(VoxelKey(1, 1, 1, 1));
            REQUIRE(sub_node.IsValid());
            auto sub_node_data = reader.GetPointData(sub_node);
            REQUIRE(sub_node_data == sixty);
        }
        {
            auto sub_node = reader.FindNode(VoxelKey(1, 0, 0, 0));
            REQUIRE(sub_node.IsValid());
            auto sub_node_data = reader.GetPointData(sub_node);
            REQUIRE(sub_node_data == twelve);
        }
        {
            auto sub_node = reader.FindNode(VoxelKey(0, 0, 0, 0));
            REQUIRE(sub_node.IsValid());
            auto sub_node_data = reader.GetPointData(sub_node);
            REQUIRE(sub_node_data == twenty);
        }
    }
}

TEST_CASE("Writer Node Compressed", "[Writer]")
{
    SECTION("Add one")
    {
        stringstream out_stream;

        Writer::LasConfig cfg(3);
        Writer writer(out_stream, cfg);

        Page root_page = writer.GetRootPage();

        REQUIRE_THROWS(writer.AddNodeCompressed(root_page, VoxelKey::InvalidKey(), std::vector<char>(), 0));

        std::vector<char> root_node(twenty_pts_prdf3_compressed,
                                    twenty_pts_prdf3_compressed + sizeof(twenty_pts_prdf3_compressed));
        REQUIRE_NOTHROW(writer.AddNodeCompressed(root_page, VoxelKey(0, 0, 0, 0), root_node, 20));

        writer.Close();

        Reader reader(out_stream);
        REQUIRE(reader.GetCopcHeader().root_hier_offset > 0);
        REQUIRE(reader.GetCopcHeader().root_hier_size == 32);

        auto node = reader.FindNode(VoxelKey::BaseKey());
        REQUIRE(node.IsValid());
        auto root_node_test = reader.GetPointData(node);

        REQUIRE(root_node_test == std::vector<char>(twenty_pts_prdf3, twenty_pts_prdf3 + sizeof(twenty_pts_prdf3)));
    }

    SECTION("Add multiple")
    {
        stringstream out_stream;

        Writer::LasConfig cfg(3);
        Writer writer(out_stream, cfg, 256, "test_wkt");

        Page root_page = writer.GetRootPage();

        std::vector<char> twenty(twenty_pts_prdf3_compressed,
                                 twenty_pts_prdf3_compressed + sizeof(twenty_pts_prdf3_compressed));
        REQUIRE_NOTHROW(writer.AddNodeCompressed(root_page, VoxelKey(0, 0, 0, 0), twenty, 20));

        std::vector<char> twelve(twelve_pts_prdf3_compressed,
                                 twelve_pts_prdf3_compressed + sizeof(twelve_pts_prdf3_compressed));
        REQUIRE_NOTHROW(writer.AddNodeCompressed(root_page, VoxelKey(1, 1, 1, 1), twelve, 12));

        std::vector<char> sixty(sixty_pts_prdf3_compressed,
                                sixty_pts_prdf3_compressed + sizeof(sixty_pts_prdf3_compressed));
        REQUIRE_NOTHROW(writer.AddNodeCompressed(root_page, VoxelKey(1, 1, 1, 0), sixty, 60));

        writer.Close();

        std::string ostr = out_stream.str();
        Reader reader(out_stream);
        REQUIRE(reader.GetCopcHeader().root_hier_offset > 0);
        REQUIRE(reader.GetCopcHeader().root_hier_size == 32 * 3);

        {
            auto sub_node = reader.FindNode(VoxelKey::BaseKey());
            REQUIRE(sub_node.IsValid());
            auto sub_node_data = reader.GetPointData(sub_node);
            REQUIRE(sub_node_data == std::vector<char>(twenty_pts_prdf3, twenty_pts_prdf3 + sizeof(twenty_pts_prdf3)));
        }

        {
            auto sub_node = reader.FindNode(VoxelKey(1, 1, 1, 1));
            REQUIRE(sub_node.IsValid());
            auto sub_node_data = reader.GetPointData(sub_node);
            REQUIRE(sub_node_data == std::vector<char>(twelve_pts_prdf3, twelve_pts_prdf3 + sizeof(twelve_pts_prdf3)));
        }

        {
            auto sub_node = reader.FindNode(VoxelKey(1, 1, 1, 0));
            REQUIRE(sub_node.IsValid());
            auto sub_node_data = reader.GetPointData(sub_node);
            REQUIRE(sub_node_data == std::vector<char>(sixty_pts_prdf3, sixty_pts_prdf3 + sizeof(sixty_pts_prdf3)));
        }
    }

    SECTION("Add hierarchy")
    {
        stringstream out_stream;

        Writer::LasConfig cfg(3);
        Writer writer(out_stream, cfg);

        Page root_page = writer.GetRootPage();
        Page sub_page1 = writer.AddSubPage(root_page, VoxelKey(1, 0, 0, 0));
        Page sub_page2 = writer.AddSubPage(root_page, VoxelKey(1, 1, 1, 1));

        std::vector<char> twenty(twenty_pts_prdf3_compressed,
                                 twenty_pts_prdf3_compressed + sizeof(twenty_pts_prdf3_compressed));
        REQUIRE_NOTHROW(writer.AddNodeCompressed(root_page, VoxelKey(0, 0, 0, 0), twenty, 20));

        std::vector<char> twelve(twelve_pts_prdf3_compressed,
                                 twelve_pts_prdf3_compressed + sizeof(twelve_pts_prdf3_compressed));
        REQUIRE_NOTHROW(writer.AddNodeCompressed(sub_page1, VoxelKey(1, 0, 0, 0), twelve, 12));

        std::vector<char> sixty(sixty_pts_prdf3_compressed,
                                sixty_pts_prdf3_compressed + sizeof(sixty_pts_prdf3_compressed));
        REQUIRE_NOTHROW(writer.AddNodeCompressed(sub_page2, VoxelKey(1, 1, 1, 1), sixty, 60));
        REQUIRE_NOTHROW(writer.AddNodeCompressed(sub_page2, VoxelKey(2, 2, 2, 2), twenty, 20));

        // Can't add a node that's not a child of the page
        REQUIRE_THROWS(writer.AddNodeCompressed(sub_page2, VoxelKey(1, 2, 2, 2), twenty, 20));

        writer.Close();

        /*
            hierarchy should look like this:
            Page 0-0-0-0
                Node 0-0-0-0
                Page 1-0-0-0
                    Node 1-0-0-0
                Page 1-1-1-1
                    Node 1-1-1-1
                    Node 2-2-2-2
        */

        Reader reader(out_stream);
        REQUIRE(reader.GetCopcHeader().root_hier_offset > 0);
        REQUIRE(reader.GetCopcHeader().root_hier_size == 32 * 3);

        {
            auto sub_node = reader.FindNode(VoxelKey(2, 2, 2, 2));
            REQUIRE(sub_node.IsValid());
            auto sub_node_data = reader.GetPointData(sub_node);
            REQUIRE(sub_node_data == std::vector<char>(twenty_pts_prdf3, twenty_pts_prdf3 + sizeof(twenty_pts_prdf3)));
        }
        {
            auto sub_node = reader.FindNode(VoxelKey(1, 1, 1, 1));
            REQUIRE(sub_node.IsValid());
            auto sub_node_data = reader.GetPointData(sub_node);
            REQUIRE(sub_node_data == std::vector<char>(sixty_pts_prdf3, sixty_pts_prdf3 + sizeof(sixty_pts_prdf3)));
        }
        {
            auto sub_node = reader.FindNode(VoxelKey(1, 0, 0, 0));
            REQUIRE(sub_node.IsValid());
            auto sub_node_data = reader.GetPointData(sub_node);
            REQUIRE(sub_node_data == std::vector<char>(twelve_pts_prdf3, twelve_pts_prdf3 + sizeof(twelve_pts_prdf3)));
        }
        {
            auto sub_node = reader.FindNode(VoxelKey(0, 0, 0, 0));
            REQUIRE(sub_node.IsValid());
            auto sub_node_data = reader.GetPointData(sub_node);
            REQUIRE(sub_node_data == std::vector<char>(twenty_pts_prdf3, twenty_pts_prdf3 + sizeof(twenty_pts_prdf3)));
        }
    }
}
