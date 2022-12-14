
EXTRA_DIST = test.sh

include_HEADERS = ref_adapt.h ref_adj.h ref_agents.h ref_args.h ref_axi.h \
	ref_cavity.h ref_cell.h ref_cloud.h \
	ref_clump.h ref_collapse.h ref_comprow.h \
	ref_dict.h ref_dist.h ref_defs.h \
	ref_edge.h ref_egads.h ref_elast.h ref_export.h \
	ref_face.h ref_facelift.h ref_fixture.h ref_fortran.h \
	ref_gather.h ref_geom.h ref_grid.h \
	ref_histogram.h ref_html.h \
	ref_import.h ref_inflate.h ref_interp.h ref_iso.h \
	ref_list.h ref_layer.h \
	ref_malloc.h \
	ref_math.h ref_matrix.h ref_meshlink.h \
	ref_metric.h ref_migrate.h ref_mpi.h \
	ref_node.h ref_oct.h ref_part.h ref_phys.h ref_recon.h \
	ref_search.h ref_shard.h ref_smooth.h ref_sort.h ref_split.h \
	ref_subdiv.h ref_swap.h ref_validation.h

lib_LIBRARIES =

noinst_LIBRARIES =

lib_LIBRARIES += librefine_core.a
librefine_core_a_SOURCES = \
	ref_adapt.c \
	ref_adj.c \
	ref_agents.c \
	ref_args.c \
	ref_axi.c \
	ref_cavity.c \
	ref_cell.c \
	ref_cloud.c \
	ref_clump.c \
	ref_collapse.c \
	ref_comprow.c \
	ref_dict.c \
	ref_dist.c \
	ref_edge.c \
	ref_elast.c \
	ref_endian.h \
	ref_export.c \
	ref_face.c \
	ref_facelift.c \
	ref_fixture.c \
	ref_fortran.c \
	ref_gather.c \
	ref_geom.c \
	ref_grid.c \
	ref_histogram.c \
	ref_html.c \
	ref_import.c \
	ref_inflate.c \
	ref_interp.c \
	ref_iso.c \
	ref_layer.c \
	ref_list.c \
	ref_math.c \
	ref_matrix.c \
	ref_metric.c \
	ref_node.c \
	ref_oct.c \
	ref_part.c \
	ref_phys.c \
	ref_recon.c \
	ref_search.c \
	ref_shard.c \
	ref_smooth.c \
	ref_sort.c \
	ref_split.c \
	ref_subdiv.c \
	ref_swap.c \
	ref_validation.c

default_ldadd = librefine_core.a

extra_cflags =
if ENABLED_SHARED
extra_cflags += -fPIC
librefine_core_a_CFLAGS = $(extra_cflags)
endif

if BUILD_MPI
lib_LIBRARIES += librefine_with_mpi.a
librefine_with_mpi_a_SOURCES = \
	ref_migrate.c \
	ref_mpi.c
librefine_with_mpi_a_CFLAGS = $(extra_cflags) -DHAVE_MPI @mpi_include@ @zoltan_include@ @parmetis_include@
endif
default_ldadd += librefine_without_mpi.a

lib_LIBRARIES += librefine_without_mpi.a
librefine_without_mpi_a_SOURCES = \
	ref_migrate.c \
	ref_mpi.c
# partitioner flags nested, safe for seq code or mpi with user CFLAG HAVE_MPI
librefine_without_mpi_a_CFLAGS = $(extra_cflags) @zoltan_include@ @parmetis_include@

if BUILD_EGADS
lib_LIBRARIES += librefine_with_egads.a
librefine_with_egads_a_SOURCES = \
	ref_egads.c
librefine_with_egads_a_CFLAGS = $(extra_cflags) -DHAVE_EGADS @egads_include@

lib_LIBRARIES += librefine_with_egadslite.a
librefine_with_egadslite_a_SOURCES = \
	ref_egads.c
librefine_with_egadslite_a_CFLAGS = $(extra_cflags) -DHAVE_EGADS -DHAVE_EGADS_LITE @egads_include@

if BUILD_EGADS_LITE
default_ldadd += librefine_with_egadslite.a
else
default_ldadd += librefine_with_egads.a
endif
egads_lib = librefine_with_egads.a
egadslite_lib = librefine_with_egadslite.a

else
default_ldadd += librefine_without_egads.a
egads_lib = librefine_without_egads.a
egadslite_lib = librefine_without_egads.a
endif

# ensure ref_egads compiles without flags
lib_LIBRARIES += librefine_without_egads.a
librefine_without_egads_a_SOURCES = \
	ref_egads.c

if ENABLED_SHARED
librefine_without_egads_a_CFLAGS = $(extra_cflags)
endif

if BUILD_MESHLINK
lib_LIBRARIES += librefine_with_meshlink.a
librefine_with_meshlink_a_SOURCES = \
	ref_meshlink.c
librefine_with_meshlink_a_CFLAGS = -DHAVE_MESHLINK @meshlink_include@
default_ldadd += librefine_with_meshlink.a
meshlink_lib = librefine_with_meshlink.a
else
default_ldadd += librefine_without_meshlink.a
meshlink_lib = librefine_without_meshlink.a
endif

# ensure ref_meshlink compiles without flags
lib_LIBRARIES += librefine_without_meshlink.a
librefine_without_meshlink_a_SOURCES = \
	ref_meshlink.c

default_ldadd += @egads_ldadd@ @opencascade_ldadd@ @meshlink_ldadd@ @zoltan_ldadd@ @parmetis_ldadd@ @compiler_rpath_ldadd@ -lm

bin_PROGRAMS =

bin_PROGRAMS += ref
ref_SOURCES = ref_subcommand.c
ref_LDADD = $(default_ldadd)

if BUILD_MPI
bin_PROGRAMS += refmpi
refmpi_SOURCES = ref_subcommand.c
refmpi_LDADD = librefine_core.a librefine_with_mpi.a $(egadslite_lib) $(meshlink_lib) @egadslite_ldadd@ @meshlink_ldadd@ @zoltan_ldadd@ @parmetis_ldadd@ @mpi_ldadd@ @compiler_rpath_ldadd@ -lm

bin_PROGRAMS += refmpifull
refmpifull_SOURCES = ref_subcommand.c
refmpifull_LDADD = librefine_core.a librefine_with_mpi.a $(egads_lib) $(meshlink_lib) @egads_ldadd@ @opencascade_ldadd@ @meshlink_ldadd@ @zoltan_ldadd@ @parmetis_ldadd@ @mpi_ldadd@ @compiler_rpath_ldadd@ -lm
endif

noinst_PROGRAMS =
TESTS =

noinst_PROGRAMS += ref_acceptance
ref_acceptance_SOURCES = ref_acceptance.c
ref_acceptance_LDADD = $(default_ldadd)

TESTS += ref_adapt_test
noinst_PROGRAMS += ref_adapt_test
ref_adapt_test_SOURCES = ref_adapt_test.c
ref_adapt_test_LDADD = $(default_ldadd)

TESTS += ref_adj_test
noinst_PROGRAMS += ref_adj_test
ref_adj_test_SOURCES = ref_adj_test.c
ref_adj_test_LDADD = $(default_ldadd)

TESTS += ref_agents_test
noinst_PROGRAMS += ref_agents_test
ref_agents_test_SOURCES = ref_agents_test.c
ref_agents_test_LDADD = $(default_ldadd)

TESTS += ref_args_test
noinst_PROGRAMS += ref_args_test
ref_args_test_SOURCES = ref_args_test.c
ref_args_test_LDADD = $(default_ldadd)

TESTS += ref_axi_test
noinst_PROGRAMS += ref_axi_test
ref_axi_test_SOURCES = ref_axi_test.c
ref_axi_test_LDADD = $(default_ldadd)

TESTS += ref_cavity_test
noinst_PROGRAMS += ref_cavity_test
ref_cavity_test_SOURCES = ref_cavity_test.c
ref_cavity_test_LDADD = $(default_ldadd)

TESTS += ref_cell_test
noinst_PROGRAMS += ref_cell_test
ref_cell_test_SOURCES = ref_cell_test.c
ref_cell_test_LDADD = $(default_ldadd)

TESTS += ref_cloud_test
noinst_PROGRAMS += ref_cloud_test
ref_cloud_test_SOURCES = ref_cloud_test.c
ref_cloud_test_LDADD = $(default_ldadd)

TESTS += ref_clump_test
noinst_PROGRAMS += ref_clump_test
ref_clump_test_SOURCES = ref_clump_test.c
ref_clump_test_LDADD = $(default_ldadd)

TESTS += ref_collapse_test
noinst_PROGRAMS += ref_collapse_test
ref_collapse_test_SOURCES = ref_collapse_test.c
ref_collapse_test_LDADD = $(default_ldadd)

TESTS += ref_comprow_test
noinst_PROGRAMS += ref_comprow_test
ref_comprow_test_SOURCES = ref_comprow_test.c
ref_comprow_test_LDADD = $(default_ldadd)

TESTS += ref_dict_test
noinst_PROGRAMS += ref_dict_test
ref_dict_test_SOURCES = ref_dict_test.c
ref_dict_test_LDADD = $(default_ldadd)

TESTS += ref_dist_test
noinst_PROGRAMS += ref_dist_test
ref_dist_test_SOURCES = ref_dist_test.c
ref_dist_test_LDADD = $(default_ldadd)

TESTS += ref_edge_test
noinst_PROGRAMS += ref_edge_test
ref_edge_test_SOURCES = ref_edge_test.c
ref_edge_test_LDADD = $(default_ldadd)

TESTS += ref_egads_test
noinst_PROGRAMS += ref_egads_test
ref_egads_test_SOURCES = ref_egads_test.c
ref_egads_test_LDADD = $(default_ldadd)

TESTS += ref_elast_test
noinst_PROGRAMS += ref_elast_test
ref_elast_test_SOURCES = ref_elast_test.c
ref_elast_test_LDADD = $(default_ldadd)

TESTS += ref_export_test
noinst_PROGRAMS += ref_export_test
ref_export_test_SOURCES = ref_export_test.c
ref_export_test_LDADD = $(default_ldadd)

TESTS += ref_face_test
noinst_PROGRAMS += ref_face_test
ref_face_test_SOURCES = ref_face_test.c
ref_face_test_LDADD = $(default_ldadd)

TESTS += ref_facelift_test
noinst_PROGRAMS += ref_facelift_test
ref_facelift_test_SOURCES = ref_facelift_test.c
ref_facelift_test_LDADD = $(default_ldadd)

TESTS += ref_fixture_test
noinst_PROGRAMS += ref_fixture_test
ref_fixture_test_SOURCES = ref_fixture_test.c
ref_fixture_test_LDADD = $(default_ldadd)

TESTS += ref_fortran_test
noinst_PROGRAMS += ref_fortran_test
ref_fortran_test_SOURCES = ref_fortran_test.c
ref_fortran_test_LDADD = $(default_ldadd)

TESTS += ref_gather_test
noinst_PROGRAMS += ref_gather_test
ref_gather_test_SOURCES = ref_gather_test.c
ref_gather_test_LDADD = $(default_ldadd)

TESTS += ref_geom_test
noinst_PROGRAMS += ref_geom_test
ref_geom_test_SOURCES = ref_geom_test.c
ref_geom_test_LDADD = $(default_ldadd)

TESTS += ref_grid_test
noinst_PROGRAMS += ref_grid_test
ref_grid_test_SOURCES = ref_grid_test.c
ref_grid_test_LDADD = $(default_ldadd)

TESTS += ref_histogram_test
noinst_PROGRAMS += ref_histogram_test
ref_histogram_test_SOURCES = ref_histogram_test.c
ref_histogram_test_LDADD = $(default_ldadd)

TESTS += ref_html_test
noinst_PROGRAMS += ref_html_test
ref_html_test_SOURCES = ref_html_test.c
ref_html_test_LDADD = $(default_ldadd)

TESTS += ref_import_test
noinst_PROGRAMS += ref_import_test
ref_import_test_SOURCES = ref_import_test.c
ref_import_test_LDADD = $(default_ldadd)

TESTS += ref_inflate_test
noinst_PROGRAMS += ref_inflate_test
ref_inflate_test_SOURCES = ref_inflate_test.c
ref_inflate_test_LDADD = $(default_ldadd)

TESTS += ref_interp_test
noinst_PROGRAMS += ref_interp_test
ref_interp_test_SOURCES = ref_interp_test.c
ref_interp_test_LDADD = $(default_ldadd)

TESTS += ref_iso_test
noinst_PROGRAMS += ref_iso_test
ref_iso_test_SOURCES = ref_iso_test.c
ref_iso_test_LDADD = $(default_ldadd)

TESTS += ref_layer_test
noinst_PROGRAMS += ref_layer_test
ref_layer_test_SOURCES = ref_layer_test.c
ref_layer_test_LDADD = $(default_ldadd)

TESTS += ref_list_test
noinst_PROGRAMS += ref_list_test
ref_list_test_SOURCES = ref_list_test.c
ref_list_test_LDADD = $(default_ldadd)

TESTS += ref_math_test
noinst_PROGRAMS += ref_math_test
ref_math_test_SOURCES = ref_math_test.c
ref_math_test_LDADD = $(default_ldadd)

TESTS += ref_matrix_test
noinst_PROGRAMS += ref_matrix_test
ref_matrix_test_SOURCES = ref_matrix_test.c
ref_matrix_test_LDADD = $(default_ldadd)

TESTS += ref_meshlink_test
noinst_PROGRAMS += ref_meshlink_test
ref_meshlink_test_SOURCES = ref_meshlink_test.c
ref_meshlink_test_LDADD = $(default_ldadd)

TESTS += ref_metric_test
noinst_PROGRAMS += ref_metric_test
ref_metric_test_SOURCES = ref_metric_test.c
ref_metric_test_LDADD = $(default_ldadd)

TESTS += ref_migrate_test
noinst_PROGRAMS += ref_migrate_test
ref_migrate_test_SOURCES = ref_migrate_test.c
ref_migrate_test_LDADD = $(default_ldadd)

TESTS += ref_mpi_test
noinst_PROGRAMS += ref_mpi_test
ref_mpi_test_SOURCES = ref_mpi_test.c
ref_mpi_test_LDADD = $(default_ldadd)

TESTS += ref_node_test
noinst_PROGRAMS += ref_node_test
ref_node_test_SOURCES = ref_node_test.c
ref_node_test_LDADD = $(default_ldadd)

TESTS += ref_oct_test
noinst_PROGRAMS += ref_oct_test
ref_oct_test_SOURCES = ref_oct_test.c
ref_oct_test_LDADD = $(default_ldadd)

TESTS += ref_part_test
noinst_PROGRAMS += ref_part_test
ref_part_test_SOURCES = ref_part_test.c
ref_part_test_LDADD = $(default_ldadd)

TESTS += ref_phys_test
noinst_PROGRAMS += ref_phys_test
ref_phys_test_SOURCES = ref_phys_test.c
ref_phys_test_LDADD = $(default_ldadd)

TESTS += ref_recon_test
noinst_PROGRAMS += ref_recon_test
ref_recon_test_SOURCES = ref_recon_test.c
ref_recon_test_LDADD = $(default_ldadd)

TESTS += ref_search_test
noinst_PROGRAMS += ref_search_test
ref_search_test_SOURCES = ref_search_test.c
ref_search_test_LDADD = $(default_ldadd)

TESTS += ref_shard_test
noinst_PROGRAMS += ref_shard_test
ref_shard_test_SOURCES = ref_shard_test.c
ref_shard_test_LDADD = $(default_ldadd)

TESTS += ref_smooth_test
noinst_PROGRAMS += ref_smooth_test
ref_smooth_test_SOURCES = ref_smooth_test.c
ref_smooth_test_LDADD = $(default_ldadd)

TESTS += ref_sort_test
noinst_PROGRAMS += ref_sort_test
ref_sort_test_SOURCES = ref_sort_test.c
ref_sort_test_LDADD = $(default_ldadd)

TESTS += ref_split_test
noinst_PROGRAMS += ref_split_test
ref_split_test_SOURCES = ref_split_test.c
ref_split_test_LDADD = $(default_ldadd)

TESTS += ref_subdiv_test
noinst_PROGRAMS += ref_subdiv_test
ref_subdiv_test_SOURCES = ref_subdiv_test.c
ref_subdiv_test_LDADD = $(default_ldadd)

TESTS += ref_swap_test
noinst_PROGRAMS += ref_swap_test
ref_swap_test_SOURCES = ref_swap_test.c
ref_swap_test_LDADD = $(default_ldadd)

TESTS += ref_validation_test
noinst_PROGRAMS += ref_validation_test
ref_validation_test_SOURCES = ref_validation_test.c
ref_validation_test_LDADD = $(default_ldadd)

