/** @file kpca_result.h
 *
 *  @author Dongryeol Lee (dongryel@cc.gatech.edu)
 */

#ifndef MLPACK_DISTRIBUTED_KPCA_KPCA_RESULT_H
#define MLPACK_DISTRIBUTED_KPCA_KPCA_RESULT_H

#include <boost/bind.hpp>
#include <boost/math/distributions/normal.hpp>
#include <boost/mpi.hpp>
#include <boost/serialization/serialization.hpp>
#include "core/metric_kernels/kernel.h"
#include "core/monte_carlo/mean_variance_pair_matrix.h"
#include "core/tree/statistic.h"
#include "core/table/table.h"

namespace mlpack {
namespace distributed_kpca {

/** @brief Represents the storage of KPCA computation results.
 */
class KpcaResult {
  private:

    // For BOOST serialization.
    friend class boost::serialization::access;

    /** @brief The lower bound on the projected KPCA projections.
     */
    core::table::DenseMatrix kpca_projections_l_;

    /** @brief The projected KPCA projections.
     */
    core::table::DenseMatrix kpca_projections_;

    /** @brief The upper bound on the projected KPCA projections.
     */
    core::table::DenseMatrix kpca_projections_u_;

    /** @brief The kernel eigenvalues.
     */
    core::table::DensePoint kernel_eigenvalues_;

    /** @brief The covariance eigenvectors.
     */
    core::table::DenseMatrix covariance_eigenvectors_;

    /** @brief The kernel principal components.
     */
    core::table::DenseMatrix kpca_components_;

  public:

    core::table::DenseMatrix &kpca_components() {
      return kpca_components_;
    }

    core::table::DensePoint &kernel_eigenvalues() {
      return kernel_eigenvalues_;
    }

    core::table::DenseMatrix &covariance_eigenvectors() {
      return covariance_eigenvectors_;
    }

    /** @brief Serialize the KPCA result object.
     */
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
      ar & kpca_projections_l_;
      ar & kpca_projections_;
      ar & kpca_projections_u_;
      ar & kernel_eigenvalues_;
      ar & covariance_eigenvectors_;
    }

    /** @brief The default constructor.
     */
    KpcaResult() {
      SetZero();
    }

    void Export(
      double num_standard_deviations,
      double mult_const,
      double correction_term_in,
      const core::monte_carlo::MeanVariancePairMatrix &kernel_sum) {
      for(int i = 0; i < kpca_projections_.n_cols(); i++) {
        for(int k = 0; k < kpca_projections_.n_rows(); k++) {
          double deviation = num_standard_deviations *
                             sqrt(kernel_sum.get(k, i).sample_mean_variance());
          kpca_projections_l_.set(
            k, i,
            (kernel_sum.get(k, i).sample_mean() -
             correction_term_in - deviation) *
            mult_const);
          kpca_projections_.set(
            k, i, (kernel_sum.get(k, i).sample_mean() - correction_term_in) *
            mult_const);
          kpca_projections_u_.set(
            k, i,
            (kernel_sum.get(k, i).sample_mean() -
             correction_term_in + deviation) * mult_const);
        }
      }
    }

    void Print(
      const std::string &kpca_components_file_name,
      const std::string &kpca_projections_file_name) const {

      FILE *file_output =
        fopen(kpca_projections_file_name.c_str(), "w+");
      for(int i = 0; i < kpca_projections_.n_cols(); i++) {
        for(int j = 0; j < kpca_projections_.n_rows(); j++) {
          fprintf(
            file_output, "(%g %g %g) ",
            kpca_projections_l_.get(j, i),
            kpca_projections_.get(j, i),
            kpca_projections_u_.get(j, i));
        }
        fprintf(file_output, "\n");
      }
      fclose(file_output);
      file_output = fopen(kpca_components_file_name.c_str(), "w+");
      for(int j = 0; j < kpca_components_.n_cols(); j++) {
        for(int i = 0; i < kpca_components_.n_rows(); i++) {
          fprintf(
            file_output, "%g ", kpca_components_.get(i, j));
        }
        fprintf(file_output, "\n");
      }
      fclose(file_output);
    }

    void Init(
      int num_components, int num_reference_points, int query_points) {
      kpca_projections_l_.Init(num_components, query_points);
      kpca_projections_.Init(num_components, query_points);
      kpca_projections_u_.Init(num_components, query_points);
      SetZero();
    }

    void SetZero() {
      kpca_projections_l_.SetZero();
      kpca_projections_.SetZero();
      kpca_projections_u_.SetZero();
    }

    void set_eigendecomposition_results(
      int num_kpca_components_in,
      const arma::vec &kernel_eigenvalues_in,
      const arma::mat &covariance_eigenvectors_in) {

      // Sort the kernel eigenvalues.
      std::vector <
      std::pair<int, double> > sorted_eigenvalues(
        kernel_eigenvalues_in.n_elem);
      for(unsigned int i = 0; i < kernel_eigenvalues_in.n_elem; i++) {
        sorted_eigenvalues[i] =
          std::pair<int, double>(i, kernel_eigenvalues_in[i]);
      }

      // Sort in the decreasing order and take the top ones.
      std::sort(
        sorted_eigenvalues.begin(), sorted_eigenvalues.end(),
        boost::bind(&std::pair<int, double>::second, _1) >
        boost::bind(&std::pair<int, double>::second, _2));

      kernel_eigenvalues_.Init(num_kpca_components_in);
      covariance_eigenvectors_.Init(
        covariance_eigenvectors_in.n_rows, num_kpca_components_in);
      for(int i = 0; i < num_kpca_components_in; i++) {
        kernel_eigenvalues_[i] = sorted_eigenvalues[i].second;
        for(unsigned int j = 0; j < covariance_eigenvectors_in.n_rows; j++) {
          covariance_eigenvectors_.set(
            j, i,
            covariance_eigenvectors_in.at(
              j, sorted_eigenvalues[i].first));
        }
      }
    }
};
}
}

#endif
