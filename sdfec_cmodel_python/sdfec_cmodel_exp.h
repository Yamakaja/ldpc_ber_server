#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include "sd_fec_v1_1_bitacc_cmodel.h"
#pragma GCC diagnostic pop

#include <cstdint>
#include <cstdlib>
#include <string>

namespace sdfec_cmodel_exp {

void init();

unsigned int gen_ldpc_params(char* path);

}; // namespace sdfec_cmodel_exp
