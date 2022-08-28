AGG_SRCS = \
	dep/agg/src/agg_arrowhead.cpp \
	dep/agg/src/agg_line_aa_basics.cpp \
	dep/agg/src/agg_vcgen_bspline.cpp \
	dep/agg/src/agg_vpgen_segmentator.cpp \
	dep/agg/src/agg_color_rgba.cpp \
	dep/agg/src/agg_sqrt_tables.cpp \
	dep/agg/src/agg_bspline.cpp \
	dep/agg/src/agg_curves.cpp \
	dep/agg/src/agg_rounded_rect.cpp \
	dep/agg/src/agg_vcgen_markers_term.cpp \
	dep/agg/src/agg_vcgen_dash.cpp \
	dep/agg/src/agg2d.cpp \
	dep/agg/src/agg_trans_affine.cpp \
	dep/agg/src/agg_gsv_text.cpp \
	dep/agg/src/agg_vcgen_smooth_poly1.cpp \
	dep/agg/src/agg_trans_single_path.cpp \
	dep/agg/src/agg_vpgen_clip_polygon.cpp \
	dep/agg/src/agg_embedded_raster_fonts.cpp \
	dep/agg/src/agg_trans_double_path.cpp \
	dep/agg/src/agg_vcgen_stroke.cpp \
	dep/agg/src/agg_arc.cpp \
	dep/agg/src/agg_image_filters.cpp \
	dep/agg/src/agg_trans_warp_magnifier.cpp \
	dep/agg/src/agg_vpgen_clip_polyline.cpp \
	dep/agg/src/agg_bezier_arc.cpp \
	dep/agg/src/agg_line_profile_aa.cpp \
	dep/agg/src/agg_vcgen_contour.cpp \
	
AGG_OBJS = $(patsubst %.cpp, $(OBJDIR)/%.o, $(AGG_SRCS))
AGG_LIB = $(OBJDIR)/libagg.a
$(AGG_LIB): $(AGG_OBJS)
AGG_LDFLAGS = $(AGG_LIB)
AGG_CFLAGS = -Idep/agg/include
OBJS += $(AGG_OBJS)

$(AGG_OBJS): CFLAGS += $(AGG_CFLAGS)

