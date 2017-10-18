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

#include <modules/userinterfacegl/glui/widgets/intpropertywidget.h>

namespace inviwo {

namespace glui {

IntPropertyWidget::IntPropertyWidget(IntProperty &property, Processor &processor,
                                     Renderer &uiRenderer, const ivec2 &extent)
    : Slider(property.getDisplayName(), property.get(), property.getMinValue(),
             property.getMaxValue(), processor, uiRenderer, extent)
    , PropertyWidget(&property)
    , property_(&property) {
    property_->addObserver(this);
    property_->registerWidget(this);

    moveAction_ = [&](const dvec2 &delta) {
        bool triggerUpdate = false;
        if (!property_->getReadOnly()) {
            // delta in pixel (screen coords),
            // need to scale from graphical representation to slider
            int newVal = static_cast<int>(
                round(getPreviousValue() +
                      delta.x / static_cast<double>(widgetExtent_.x - widgetExtent_.y) *
                          static_cast<double>(getMaxValue() - getMinValue())));
            if (newVal != property_->get()) {
                property_->set(newVal);
                triggerUpdate = true;
            }
        }
        return triggerUpdate;
    };
    updateFromProperty();
}

void IntPropertyWidget::updateFromProperty() {
    set(property_->get(), property_->getMinValue(), property_->getMaxValue());
}

void IntPropertyWidget::onSetVisible(bool visible) { setVisible(visible); }

void IntPropertyWidget::onSetDisplayName(const std::string &displayName) {
    setLabel(displayName);
    property_->propertyModified();
}

}  // namespace glui

}  // namespace inviwo
