/*
 * CombiMinMaxScheme.hpp
 *
 *  Created on: Oct 2, 2015
 *      Author: sccs
 */

#ifndef SRC_SGPP_COMBIGRID_COMBISCHEME_STATICCOMBISCHEME_HPP_
#define SRC_SGPP_COMBIGRID_COMBISCHEME_STATICCOMBISCHEME_HPP_

#include "sgpp/distributedcombigrid/utils/Types.hpp"
#include "sgpp/distributedcombigrid/utils/LevelVector.hpp"
#include <boost/math/special_functions/binomial.hpp>
#include <numeric>
#include <vector>
#include <algorithm>

namespace combigrid {

class StaticCombiScheme {
public:

	static StaticCombiScheme createClassicalScheme(LevelVector lmin, LevelVector lmax, bool faultTolerant = false){
		return StaticCombiScheme(makeClassicalMin(lmin, lmax), lmax, faultTolerant);
  }

	static StaticCombiScheme createAdaptiveScheme(LevelVector lmin, LevelVector lmax, bool faultTolerant = false){
		return StaticCombiScheme(lmin, lmax, faultTolerant);
  }

  /* Generate the combischeme corresponding to the classical combination technique.
   * We need to ensure that lmax = lmin +c*ones(dim), and take special care
   * of dummy dimensions
   * */
  void createCombiScheme();

  static LevelVector makeClassicalMin(LevelVector lmin, LevelVector lmax); 

  inline const std::vector<LevelVector>& getCombiSpaces() const {
    return combiSpaces_;
  }

  inline const std::vector<double>& getCoeffs() const {
    return coefficients_;
  }

  const std::vector<LevelVector>& getLevels() const{
	 return levels_;
  };

  inline void print(std::ostream& os) const;

  LevelType dim(){
    return static_cast<LevelType>(lmin_.size());
  }

  std::size_t effDim(){
	  std::size_t count = 0;
	  for(size_t i = 0; i < dim(); ++i){
		  if(lmin_.at(i) != lmax_.at(i)){
			  ++count;
		  }
	  }
	  return count;
  }

  bool isDummyDim(size_t index){
    assert(index < dim());
    return lmax_.at(index) == lmin_.at(index);
  }

protected:
  /* Minimal resolution */
  LevelVector lmin_;

  /* Maximal resolution */
  LevelVector lmax_;

  /* Downset */
  std::vector<LevelVector> levels_;

  /* Subspaces of the combination technique*/
  std::vector<LevelVector> combiSpaces_;

  /* Combination coefficients */
  std::vector<real> coefficients_;

  StaticCombiScheme(const LevelVector& lmin, const LevelVector& lmax, bool faultTolerant) :
  lmin_ {lmin}, lmax_{lmax}, levels_{}, combiSpaces_{}, coefficients_{}{
	assert(lmin_.size() > 0);
    assert(lmax_.size() == lmin_.size());

    for (size_t i = 0; i < dim(); ++i) {
      assert(lmax_[i] > 0);
      assert(lmin_[i] > 0);
      assert(lmax_[i] >= lmin_[i]);
    }

    createLevelsRec();
    computeCombiCoeffs();

    if(faultTolerant){
    	makeFaultTolerant();
    }
  }

  /**
   * Returns the maximum possible sum of the levels of the grids
   * that are contained in this scheme
   */
  LevelType getMaxLevelSum();

  LevelType getNameSum();

  void createLevelsRec(){
	  createLevelsRec(getNameSum(), dim() - 1, LevelVector(dim(), 0));
	  const LevelVector normalizer = lmin_ - LevelVector(dim(), 1);
	  for(auto& level : levels_){
		  level = level + normalizer;
	  }
  }

  void makeFaultTolerant();

  /* Creates the downset recursively */
  void createLevelsRec(LevelType currMaxSum,
          size_t currentIndex,
          LevelVector l);

  /* Calculate the coefficients of the classical CT (binomial coefficient)*/
  void computeCombiCoeffs();


};


inline std::ostream& operator<<(std::ostream& os,
    const combigrid::StaticCombiScheme& scheme) {
  scheme.print(os);
  return os;
}


inline void StaticCombiScheme::print(std::ostream& os) const {
  for (uint i = 0; i < combiSpaces_.size(); ++i)
    os << "\t" << i << ". "<< combiSpaces_[i] << "\t" << coefficients_[i] << std::endl;

  os << std::endl;
}

}
#endif /* SRC_SGPP_COMBIGRID_COMBISCHEME_COMBIMINMAXSCHEME_HPP_ */