/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2017 Inviwo Foundation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *********************************************************************************/

#include <modules/userinterfacegl/glui/element.h>
#include <modules/userinterfacegl/glui/renderer.h>

#include <inviwo/core/util/exception.h>
#include <inviwo/core/util/moduleutils.h>
#include <modules/fontrendering/textrenderer.h>
#include <modules/opengl/openglutils.h>

#include <numeric>

namespace inviwo {

namespace glui {

Element::Element(const std::string &label, Processor &processor, Renderer &uiRenderer)
    : action_([]() {})
    , moveAction_([](const dvec2 &) { return false; })
    , hovered_(false)
    , pushed_(false)
    , checked_(false)
    , visible_(true)
    , boldLabel_(false)
    , labelDirty_(true)
    , processor_(&processor)
    , uiRenderer_(&uiRenderer)
    , pickingMapper_(&processor, 1, [&](PickingEvent *e) { handlePickingEvent(e); }) {
    setLabel(label);
}

Element::~Element() = default;

void Element::setLabel(const std::string &str) {
    if (str != labelStr_) {
        labelStr_ = str;

        // update label extent
        if (!labelStr_.empty()) {
            labelExtent_ =
                ivec2(uiRenderer_->getTextRenderer(boldLabel_).computeTextSize(labelStr_));
        } else {
            labelExtent_ = ivec2(0, 0);
        }
        labelDirty_ = true;
        processor_->invalidate(InvalidationLevel::InvalidOutput);
    }
}

const std::string &Element::getLabel() const { return labelStr_; }

bool Element::isDirty() const { return labelDirty_; }

void Element::setVisible(bool visible /*= true*/) { visible_ = visible; }

bool Element::isVisible() const { return visible_; }

void Element::setLabelBold(bool bold) {
    if (boldLabel_ != bold) {
        boldLabel_ = bold;

        if (!labelStr_.empty()) {
            labelExtent_ =
                ivec2(uiRenderer_->getTextRenderer(boldLabel_).computeTextSize(labelStr_));
            labelDirty_ = true;
            processor_->invalidate(InvalidationLevel::InvalidOutput);
        }
    }
}

bool Element::isLabelBold() const { return boldLabel_; }

const ivec2 &Element::getExtent() {
    if (labelDirty_) {
        updateExtent();
    }
    return extent_;
}

void Element::render(const ivec2 &origin, const ivec2 &canvasDim) {
    utilgl::DepthFuncState depthFunc(GL_ALWAYS);
    utilgl::BlendModeState blendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    auto &uiShader = uiRenderer_->getShader();
    uiShader.activate();

    uiShader.setUniform("outportParameters.dimensions", vec2(canvasDim));
    uiShader.setUniform("outportParameters.reciprocalDimensions", vec2(1.0f) / vec2(canvasDim));

    uiShader.setUniform("uiColor", uiRenderer_->getUIColor());
    uiShader.setUniform("haloColor", uiRenderer_->getHoverColor());

    renderWidget(origin);

    uiShader.deactivate();

    renderLabel(origin, canvasDim);
}

void Element::setHoverState(bool enable) {
    hovered_ = enable;
    if (!enable) {
        // reset the pushed flag when hovering ends
        pushed_ = false;
    }
}

void Element::setPushedState(bool pushed) {
    pushed_ = pushed;
    pushStateChanged();
}

bool Element::isPushed() const { return pushed_; }

void Element::setAction(const std::function<void()> &action) { action_ = action; }

void Element::triggerAction() {
    updateState();
    action_();
}

void Element::setMouseMoveAction(const std::function<bool(const dvec2 &)> &action) {
    moveAction_ = action;
}

bool Element::moveAction(const dvec2 &delta) { return moveAction_(delta); }

Element::UIState Element::uiState() const { return UIState::Normal; }

vec2 Element::marginScale() const { return vec2(1.0f); }

void Element::updateExtent() {
    if (labelDirty_) {
        updateLabelPos();
    }
    extent_ = glm::max(widgetPos_ + widgetExtent_, labelPos_ + labelExtent_);
}

void Element::updateLabelPos() {
    if (!labelStr_.empty()) {
        // compute label position
        // keep track of the font descent to account for proper centering
        int fontDescent = uiRenderer_->getTextRenderer(boldLabel_).getBaseLineDescent();
        labelPos_ = computeLabelPos(fontDescent);
    } else {
        labelPos_ = ivec2(0, 0);
    }
}

void Element::updateLabel() {
    const vec4 black(0.0f, 0.0f, 0.0f, 1.0f);

    updateLabelPos();

    if (!labelStr_.empty()) {
        size2_t labelSize(labelExtent_);
        if (!labelTexture_ || (labelTexture_->getDimensions() != labelSize)) {
            auto texture = std::make_shared<Texture2D>(labelSize, GL_RGBA, GL_RGBA,
                                                       GL_UNSIGNED_BYTE, GL_LINEAR);
            texture->initialize(nullptr);
            labelTexture_ = texture;
        }
        uiRenderer_->getTextRenderer(boldLabel_).renderToTexture(labelTexture_, labelStr_, black);
    } else {
        labelTexture_ = nullptr;
    }

    labelDirty_ = false;
}

void Element::handlePickingEvent(PickingEvent *e) {
    if (e->getEvent()->hash() == MouseEvent::chash()) {
        auto mouseEvent = e->getEventAs<MouseEvent>();

        bool leftMouseBtn = (mouseEvent->button() == MouseButton::Left);
        bool mousePress = (mouseEvent->state() == MouseState::Press);
        bool mouseRelease = (mouseEvent->state() == MouseState::Release);
        bool mouseMove = (mouseEvent->state() == MouseState::Move);

        bool triggerUpdate = false;
        if (e->getState() == PickingState::Started) {
            setHoverState(true);
            triggerUpdate = true;
        } else if (e->getState() == PickingState::Finished) {
            setHoverState(false);
            triggerUpdate = true;
        } else if (e->getState() == PickingState::Updated) {
            if (mouseMove && (mouseEvent->buttonState() & MouseButton::Left) == MouseButton::Left) {
                auto delta = e->getDeltaPressedPosition();
                triggerUpdate = moveAction(delta * dvec2(e->getCanvasSize()));

                e->markAsUsed();
            } else if (leftMouseBtn) {
                if (mousePress) {
                    // initial activation with mouse button press
                    setPushedState(true);
                    triggerUpdate = true;
                    e->markAsUsed();
                } else if (mouseRelease && isPushed()) {
                    // mouse button is release upon the active element
                    triggerAction();
                    setPushedState(false);
                    triggerUpdate = true;
                    e->markAsUsed();
                }
            }
        }
        if (triggerUpdate) {
            processor_->invalidate(InvalidationLevel::InvalidOutput);
        }
    }
}

void Element::renderLabel(const ivec2 &origin, const size2_t &canvasDim) {
    if (labelDirty_) {
        updateLabel();
    }
    if (labelTexture_) {
        uiRenderer_->getTextureQuadRenderer().renderToRect(labelTexture_, origin + labelPos_,
                                                           labelExtent_, canvasDim);
    }
}

}  // namespace glui

}  // namespace inviwo
