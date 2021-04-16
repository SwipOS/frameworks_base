/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <SkAndroidFrameworkUtils.h>
#include <SkCanvas.h>
#include <SkPath.h>
#include <SkRegion.h>
#include <SkVertices.h>
#include <SkImage.h>
#include <SkPicture.h>
#include <SkRuntimeEffect.h>

#include <log/log.h>

#include "hwui/Bitmap.h"
#include "CanvasProperty.h"
#include "CanvasOpTypes.h"
#include "Layer.h"
#include "Points.h"
#include "RenderNode.h"

#include <experimental/type_traits>
#include <utility>

namespace android::uirenderer {

template <CanvasOpType T>
struct CanvasOp;

struct CanvasOpTraits {
    CanvasOpTraits() = delete;

    template<class T>
    using draw_t = decltype(std::integral_constant<void (T::*)(SkCanvas*) const, &T::draw>{});

    template <class T>
    static constexpr bool can_draw = std::experimental::is_detected_v<draw_t, T>;
};

#define ASSERT_DRAWABLE() private: constexpr void _check_drawable() \
    { static_assert(CanvasOpTraits::can_draw<std::decay_t<decltype(*this)>>); }

// ----------------------------------------------
//   State Ops
//  ---------------------------------------------

template <>
struct CanvasOp<CanvasOpType::Save> {
    void draw(SkCanvas* canvas) const { canvas->save(); }
    ASSERT_DRAWABLE()
};

template <>
struct CanvasOp<CanvasOpType::SaveLayer> {
    SkCanvas::SaveLayerRec saveLayerRec;
    void draw(SkCanvas* canvas) const { canvas->saveLayer(saveLayerRec); }
    ASSERT_DRAWABLE()
};

template <>
struct CanvasOp<CanvasOpType::SaveBehind> {
    SkRect bounds;
    void draw(SkCanvas* canvas) const { SkAndroidFrameworkUtils::SaveBehind(canvas, &bounds); }
    ASSERT_DRAWABLE()
};

template <>
struct CanvasOp<CanvasOpType::Restore> {
    void draw(SkCanvas* canvas) const { canvas->restore(); }
    ASSERT_DRAWABLE()
};

template <>
struct CanvasOp<CanvasOpType::BeginZ> {
};
template <>
struct CanvasOp<CanvasOpType::EndZ> {};

// ----------------------------------------------
//   Clip Ops
//  ---------------------------------------------

template <>
struct CanvasOp<CanvasOpType::ClipRect> {
    SkRect rect;
    SkClipOp clipOp;
    void draw(SkCanvas* canvas) const { canvas->clipRect(rect, clipOp); }
    ASSERT_DRAWABLE()
};

template <>
struct CanvasOp<CanvasOpType::ClipPath> {
    SkPath path;
    SkClipOp op;
    void draw(SkCanvas* canvas) const { canvas->clipPath(path, op, true); }
    ASSERT_DRAWABLE()
};

// ----------------------------------------------
//   Drawing Ops
//  ---------------------------------------------

template<>
struct CanvasOp<CanvasOpType::DrawRoundRectProperty> {
    sp<uirenderer::CanvasPropertyPrimitive> left;
    sp<uirenderer::CanvasPropertyPrimitive> top;
    sp<uirenderer::CanvasPropertyPrimitive> right;
    sp<uirenderer::CanvasPropertyPrimitive> bottom;
    sp<uirenderer::CanvasPropertyPrimitive> rx;
    sp<uirenderer::CanvasPropertyPrimitive> ry;
    sp<uirenderer::CanvasPropertyPaint> paint;

    void draw(SkCanvas* canvas) const {
        SkRect rect = SkRect::MakeLTRB(left->value, top->value, right->value, bottom->value);
        canvas->drawRoundRect(rect, rx->value, ry->value, paint->value);
    }
    ASSERT_DRAWABLE()
};

template<>
struct CanvasOp<CanvasOpType::DrawCircleProperty> {
    sp<uirenderer::CanvasPropertyPrimitive> x;
    sp<uirenderer::CanvasPropertyPrimitive> y;
    sp<uirenderer::CanvasPropertyPrimitive> radius;
    sp<uirenderer::CanvasPropertyPaint> paint;

    void draw(SkCanvas* canvas) const {
        canvas->drawCircle(x->value, y->value, radius->value, paint->value);
    }
    ASSERT_DRAWABLE()
};

template<>
struct CanvasOp<CanvasOpType::DrawRippleProperty> {
    sp<uirenderer::CanvasPropertyPrimitive> x;
    sp<uirenderer::CanvasPropertyPrimitive> y;
    sp<uirenderer::CanvasPropertyPrimitive> radius;
    sp<uirenderer::CanvasPropertyPaint> paint;
    sp<uirenderer::CanvasPropertyPrimitive> progress;
    sp<uirenderer::CanvasPropertyPrimitive> turbulencePhase;
    sk_sp<SkRuntimeEffect> effect;

    const float PI = 3.1415926535897932384626;
    const float PI_ROTATE_RIGHT = PI * 0.0078125;
    const float PI_ROTATE_LEFT = PI * -0.0078125;
    const float SCALE = 1.5;
    const float CIRCLE_X_1 = 0.01 * cos(SCALE * 0.55);
    const float CIRCLE_Y_1 = 0.01 * sin(SCALE * 0.55);
    const float CIRCLE_X_2 = -0.0066 * cos(SCALE * 0.45);
    const float CIRCLE_Y_2 = -0.0066 * sin(SCALE * 0.45);
    const float CIRCLE_X_3 = -0.0066 * cos(SCALE * 0.35);
    const float CIRCLE_Y_3 = -0.0066 * sin(SCALE * 0.35);

    void draw(SkCanvas* canvas) const {
        SkRuntimeShaderBuilder runtimeEffectBuilder(effect);

        setUniform2f(runtimeEffectBuilder, "in_origin", x->value, y->value);
        setUniform(runtimeEffectBuilder, "in_radius", radius);
        setUniform(runtimeEffectBuilder, "in_progress", progress);
        setUniform(runtimeEffectBuilder, "in_turbulencePhase", turbulencePhase);

        //
        // Keep in sync with:
        // frameworks/base/graphics/java/android/graphics/drawable/RippleShader.java
        //
        const float turbulence = turbulencePhase->value;
        setUniform2f(runtimeEffectBuilder, "in_tCircle1", SCALE * 0.5 + (turbulence * CIRCLE_X_1),
                     SCALE * 0.5 + (turbulence * CIRCLE_Y_1));
        setUniform2f(runtimeEffectBuilder, "in_tCircle2", SCALE * 0.2 + (turbulence * CIRCLE_X_2),
                     SCALE * 0.2 + (turbulence * CIRCLE_Y_2));
        setUniform2f(runtimeEffectBuilder, "in_tCircle3", SCALE + (turbulence * CIRCLE_X_3),
                     SCALE + (turbulence * CIRCLE_Y_3));
        const float rotation1 = turbulence * PI_ROTATE_RIGHT + 1.7 * PI;
        setUniform2f(runtimeEffectBuilder, "in_tRotation1", cos(rotation1), sin(rotation1));
        const float rotation2 = turbulence * PI_ROTATE_LEFT + 2 * PI;
        setUniform2f(runtimeEffectBuilder, "in_tRotation2", cos(rotation2), sin(rotation2));
        const float rotation3 = turbulence * PI_ROTATE_RIGHT + 2.75 * PI;
        setUniform2f(runtimeEffectBuilder, "in_tRotation3", cos(rotation3), sin(rotation3));

        SkPaint paintMod = paint->value;
        paintMod.setShader(runtimeEffectBuilder.makeShader(nullptr, false));
        canvas->drawCircle(x->value, y->value, radius->value, paintMod);
    }

    void setUniform(SkRuntimeShaderBuilder& effect, std::string name,
                    sp<uirenderer::CanvasPropertyPrimitive> property) const {
        SkRuntimeShaderBuilder::BuilderUniform uniform = effect.uniform(name.c_str());
        if (uniform.fVar != nullptr) {
            uniform = property->value;
        }
    }

    void setUniform2f(SkRuntimeShaderBuilder effect, std::string name, float a, float b) const {
        SkRuntimeShaderBuilder::BuilderUniform uniform = effect.uniform(name.c_str());
        if (uniform.fVar != nullptr) {
            uniform = SkV2{a, b};
        }
    }

    ASSERT_DRAWABLE()
};

template <>
struct CanvasOp<CanvasOpType::DrawColor> {
    SkColor4f color;
    SkBlendMode mode;
    void draw(SkCanvas* canvas) const { canvas->drawColor(color, mode); }
    ASSERT_DRAWABLE()
};

template <>
struct CanvasOp<CanvasOpType::DrawPaint> {
    SkPaint paint;
    void draw(SkCanvas* canvas) const { canvas->drawPaint(paint); }
    ASSERT_DRAWABLE()
};

template <>
struct CanvasOp<CanvasOpType::DrawPoint> {
    float x;
    float y;
    SkPaint paint;
    void draw(SkCanvas* canvas) const { canvas->drawPoint(x, y, paint); }
    ASSERT_DRAWABLE()
};

template <>
struct CanvasOp<CanvasOpType::DrawPoints> {
    size_t count;
    SkPaint paint;
    sk_sp<Points> points;
    void draw(SkCanvas* canvas) const {
        canvas->drawPoints(
            SkCanvas::kPoints_PointMode,
            count,
            points->data(),
            paint
        );
    }
    ASSERT_DRAWABLE()
};

template <>
struct CanvasOp<CanvasOpType::DrawRect> {
    SkRect rect;
    SkPaint paint;
    void draw(SkCanvas* canvas) const { canvas->drawRect(rect, paint); }
    ASSERT_DRAWABLE()
};

template <>
struct CanvasOp<CanvasOpType::DrawRegion> {
    SkRegion region;
    SkPaint paint;
    void draw(SkCanvas* canvas) const { canvas->drawRegion(region, paint); }
    ASSERT_DRAWABLE()
};

template<>
struct CanvasOp<CanvasOpType::DrawRoundRect> {
    SkRect rect;
    SkScalar rx;
    SkScalar ry;
    SkPaint paint;
    void draw(SkCanvas* canvas) const {
        canvas->drawRoundRect(rect, rx, ry, paint);
    }
    ASSERT_DRAWABLE()
};

template<>
struct CanvasOp<CanvasOpType::DrawDoubleRoundRect> {
    SkRRect outer;
    SkRRect inner;
    SkPaint paint;
    void draw(SkCanvas* canvas) const {
        canvas->drawDRRect(outer, inner, paint);
    }
    ASSERT_DRAWABLE()
};

template<>
struct CanvasOp<CanvasOpType::DrawCircle> {
    SkScalar cx;
    SkScalar cy;
    SkScalar radius;
    SkPaint paint;
    void draw(SkCanvas* canvas) const {
        canvas->drawCircle(cx, cy, radius, paint);
    }
    ASSERT_DRAWABLE()
};

template<>
struct CanvasOp<CanvasOpType::DrawOval> {
    SkRect oval;
    SkPaint paint;
    void draw(SkCanvas* canvas) const {
        canvas->drawOval(oval, paint);
    }
    ASSERT_DRAWABLE()
};

template<>
struct CanvasOp<CanvasOpType::DrawArc> {
    SkRect oval;
    SkScalar startAngle;
    SkScalar sweepAngle;
    bool useCenter;
    SkPaint paint;

    void draw(SkCanvas* canvas) const {
        canvas->drawArc(oval, startAngle, sweepAngle, useCenter, paint);
    }
    ASSERT_DRAWABLE()
};

template<>
struct CanvasOp<CanvasOpType::DrawPath> {
    SkPath path;
    SkPaint paint;

    void draw(SkCanvas* canvas) const { canvas->drawPath(path, paint); }
    ASSERT_DRAWABLE()
};

template<>
struct CanvasOp<CanvasOpType::DrawLine> {
    float startX;
    float startY;
    float endX;
    float endY;
    SkPaint paint;

    void draw(SkCanvas* canvas) const {
        canvas->drawLine(startX, startY, endX, endY, paint);
    }
    ASSERT_DRAWABLE()
};

template<>
struct CanvasOp<CanvasOpType::DrawLines> {
    size_t count;
    SkPaint paint;
    sk_sp<Points> points;
    void draw(SkCanvas* canvas) const {
        canvas->drawPoints(
            SkCanvas::kLines_PointMode,
            count,
            points->data(),
            paint
        );
    }
    ASSERT_DRAWABLE()
};

template<>
struct CanvasOp<CanvasOpType::DrawVertices> {
    sk_sp<SkVertices> vertices;
    SkBlendMode mode;
    SkPaint paint;
    void draw(SkCanvas* canvas) const {
        canvas->drawVertices(vertices, mode, paint);
    }
    ASSERT_DRAWABLE()
};

template<>
struct CanvasOp<CanvasOpType::DrawImage> {

    CanvasOp(
        const sk_sp<Bitmap>& bitmap,
        float left,
        float top,
        SkFilterMode filter,
        SkPaint paint
    ) : left(left),
        top(top),
        filter(filter),
        paint(std::move(paint)),
        bitmap(bitmap),
        image(bitmap->makeImage()) { }

    float left;
    float top;
    SkFilterMode filter;
    SkPaint paint;
    sk_sp<Bitmap> bitmap;
    sk_sp<SkImage> image;

    void draw(SkCanvas* canvas) const {
        canvas->drawImage(image, left, top, SkSamplingOptions(filter), &paint);
    }
    ASSERT_DRAWABLE()
};

template<>
struct CanvasOp<CanvasOpType::DrawImageRect> {

    CanvasOp(
        const sk_sp<Bitmap>& bitmap,
        SkRect src,
        SkRect dst,
        SkFilterMode filter,
        SkPaint paint
    ) : src(src),
        dst(dst),
        filter(filter),
        paint(std::move(paint)),
        bitmap(bitmap),
        image(bitmap->makeImage()) { }

    SkRect src;
    SkRect dst;
    SkFilterMode filter;
    SkPaint paint;
    sk_sp<Bitmap> bitmap;
    sk_sp<SkImage> image;

    void draw(SkCanvas* canvas) const {
        canvas->drawImageRect(image,
                src,
                dst,
                SkSamplingOptions(filter),
                &paint,
                SkCanvas::kFast_SrcRectConstraint
        );
    }
    ASSERT_DRAWABLE()
};

template<>
struct CanvasOp<CanvasOpType::DrawImageLattice> {

    CanvasOp(
        const sk_sp<Bitmap>& bitmap,
        SkRect dst,
        SkCanvas::Lattice lattice,
        SkFilterMode filter,
        SkPaint  paint
    ):  dst(dst),
        lattice(lattice),
        filter(filter),
        bitmap(bitmap),
        image(bitmap->makeImage()),
        paint(std::move(paint)) {}

    SkRect dst;
    SkCanvas::Lattice lattice;
    SkFilterMode filter;
    const sk_sp<Bitmap> bitmap;
    const sk_sp<SkImage> image;

    SkPaint paint;
    void draw(SkCanvas* canvas) const {
        canvas->drawImageLattice(image.get(), lattice, dst, filter, &paint);
    }
    ASSERT_DRAWABLE()
};

template<>
struct CanvasOp<CanvasOpType::DrawPicture> {
    sk_sp<SkPicture> picture;
    void draw(SkCanvas* canvas) const {
        picture->playback(canvas);
    }
};

template<>
struct CanvasOp<CanvasOpType::DrawLayer> {
    sp<Layer> layer;
};

template<>
struct CanvasOp<CanvasOpType::DrawRenderNode> {
    sp<RenderNode> renderNode;
};

// cleanup our macros
#undef ASSERT_DRAWABLE

}  // namespace android::uirenderer
