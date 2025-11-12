import {
  makeThemeProps
} from "./chunk-CEVB7HK3.js";
import {
  EventProp,
  genericComponent,
  makeComponentProps,
  propsFactory,
  useRender
} from "./chunk-QPYD2SUY.js";
import {
  createBaseVNode,
  normalizeClass,
  normalizeStyle
} from "./chunk-ZS4T33C3.js";

// node_modules/vuetify/lib/components/VLabel/VLabel.js
import "/app/node_modules/vuetify/lib/components/VLabel/VLabel.css";
var makeVLabelProps = propsFactory({
  text: String,
  onClick: EventProp(),
  ...makeComponentProps(),
  ...makeThemeProps()
}, "VLabel");
var VLabel = genericComponent()({
  name: "VLabel",
  props: makeVLabelProps(),
  setup(props, _ref) {
    let {
      slots
    } = _ref;
    useRender(() => {
      var _a;
      return createBaseVNode("label", {
        "class": normalizeClass(["v-label", {
          "v-label--clickable": !!props.onClick
        }, props.class]),
        "style": normalizeStyle(props.style),
        "onClick": props.onClick
      }, [props.text, (_a = slots.default) == null ? void 0 : _a.call(slots)]);
    });
    return {};
  }
});

export {
  VLabel
};
//# sourceMappingURL=chunk-T3H6SUKO.js.map
