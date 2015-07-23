/*
 * IOBenchmark.cpp
 *
 *  Created on: 01.02.2013
 *      Author: Christian Staudt (christian.staudt@kit.edu)
 */

#ifndef NOGTEST

#include "IOBenchmark.h"
#include "../RasterReader.h"
#include "../../generators/Quadtree/QuadtreePolarEuclid.h"
#include "../../geometric/HyperbolicSpace.h"

namespace NetworKit {



TEST_F(IOBenchmark, timeMETISGraphReader) {
	std::string path = "";

	std::cout << "[INPUT] .graph file path >" << std::endl;
	std::getline(std::cin, path);

	Aux::Timer runtime;

	INFO("[BEGIN] reading graph: " , path);
	runtime.start();
	METISGraphReader reader;
	Graph G = reader.read(path);
	runtime.stop();
	INFO("[DONE] reading graph " , runtime.elapsedTag());

	EXPECT_TRUE(! G.isEmpty());

}


TEST_F(IOBenchmark, benchRasterReader) {
	double normalizationFactor = 0.05;
	RasterReader reader(normalizationFactor);
	std::vector<double> xcoords;
	std::vector<double> ycoords;
	Aux::Timer runtime;

	std::vector<std::string> countries = {"deu", "fra", "usa"};
	for (auto country: countries) {
		std::string path("input/" + country + "p00ag.asc");

		// read raster file
		INFO("[BEGIN] reading raster data set: ", path);
		runtime.start();
		std::tie(xcoords, ycoords) = reader.read(path);
		runtime.stop();
		INFO("[DONE] reading raster data set " , runtime.elapsedTag());
		EXPECT_EQ(xcoords.size(), ycoords.size());

		count numRuns = 10;

		for (index run = 0; run < numRuns; run++) {
			//transform into polar coordinates
			runtime.start();
			vector<double> angles(xcoords.size());
			vector<double> radii(xcoords.size());
			vector<index> content(xcoords.size());
			double maxR = 0;
			for (index i = 0; i < xcoords.size(); i++) {
				HyperbolicSpace::cartesianToPolar(Point2D<double>(xcoords[i], ycoords[i]), angles[i], radii[i]);
				maxR = std::max(maxR, radii[i]);
				content[i] = i;
			}
			runtime.stop();

			INFO("Converted coordinates", runtime.elapsedTag());
			//define query function
			double T = 0.01;
			auto edgeProb = [](double distance) -> double {return exp(-(distance*200+5));};
			//auto edgeProb = [beta, thresholdDistance](double distance) -> double {return 1 / (exp(beta*(distance-thresholdDistance)/2)+1);};

			// perform range queries
			runtime.start();
			QuadtreePolarEuclid<index> tree(angles, radii, content);
			runtime.stop();
			INFO("Filled quadtree", runtime.elapsedTag());

			uint64_t numQueries = 1000;
			long treeTotalNeighbours = 0;

			runtime.start();
			for (uint64_t q = 0; q < numQueries; ++q) {
				vector<index> result;
				index comparison = Aux::Random::integer(xcoords.size());
				Point2D<double> query(xcoords[comparison], ycoords[comparison]);
				tree.getElementsProbabilistically(query, edgeProb, result);
				treeTotalNeighbours += result.size();
			}
			runtime.stop();
			INFO("Completed ", numQueries, " quadtree queries with on average ", treeTotalNeighbours / numQueries, " neighbours", runtime.elapsedTag());

			long naiveTotalNeighbours = 0;
			runtime.start();
			for (uint64_t q = 0; q < numQueries; ++q) {
				vector<index> result;
				index comparison = Aux::Random::integer(xcoords.size());
				double x = xcoords[comparison];
				double y = ycoords[comparison];
				for (index i = 0; i < xcoords.size(); i++) {
					double xdiff = xcoords[i] - x;
					double ydiff = ycoords[i] - y;
					double prob = edgeProb(pow(xdiff*xdiff+ydiff*ydiff , 0.5));
					double random = Aux::Random::real();
					if (random < prob) result.push_back(i);
				}
				naiveTotalNeighbours += result.size();
			}
			runtime.stop();
			INFO("Completed ", numQueries, " naive queries with on average ", naiveTotalNeighbours / numQueries, " neighbours", runtime.elapsedTag());
			EXPECT_NEAR(treeTotalNeighbours, naiveTotalNeighbours, treeTotalNeighbours / 10);
		}
	}
}


} /* namespace NetworKit */


#endif /* NOGTEST */
