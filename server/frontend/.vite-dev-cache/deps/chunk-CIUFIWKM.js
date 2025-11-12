import {
  VSelectionControl,
  makeVSelectionControlProps
} from "./chunk-NWW3IG4H.js";
import {
  genericComponent,
  propsFactory,
  useRender
} from "./chunk-QPYD2SUY.js";
import {
  createVNode,
  mergeProps
} from "./chunk-ZS4T33C3.js";

// node_modules/vuetify/lib/components/VRadio/VRadio.js
var makeVRadioProps = propsFactory({
  ...makeVSelectionControlProps({
    falseIcon: "$radioOff",
    trueIcon: "$radioOn"
  })
}, "VRadio");
var VRadio = genericComponent()({
  name: "VRadio",
  props: makeVRadioProps(),
  setup(props, _ref) {
    let {
      slots
    } = _ref;
    useRender(() => {
      const controlProps = VSelectionControl.filterProps(props);
      return createVNode(VSelectionControl, mergeProps(controlProps, {
        "class": ["v-radio", props.class],
        "style": props.style,
        "type": "radio"
      }), slots);
    });
    return {};
  }
});

export {
  VRadio
};
//# sourceMappingURL=chunk-CIUFIWKM.js.map
