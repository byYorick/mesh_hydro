import {
  useColor
} from "./chunk-XL4SHUX5.js";
import {
  getCurrentInstanceName,
  propsFactory
} from "./chunk-QPYD2SUY.js";
import {
  Fragment,
  createBaseVNode,
  normalizeClass,
  toRef,
  toValue
} from "./chunk-ZS4T33C3.js";

// node_modules/vuetify/lib/composables/variant.js
var allowedVariants = ["elevated", "flat", "tonal", "outlined", "text", "plain"];
function genOverlays(isClickable, name) {
  return createBaseVNode(Fragment, null, [isClickable && createBaseVNode("span", {
    "key": "overlay",
    "class": normalizeClass(`${name}__overlay`)
  }, null), createBaseVNode("span", {
    "key": "underlay",
    "class": normalizeClass(`${name}__underlay`)
  }, null)]);
}
var makeVariantProps = propsFactory({
  color: String,
  variant: {
    type: String,
    default: "elevated",
    validator: (v) => allowedVariants.includes(v)
  }
}, "variant");
function useVariant(props) {
  let name = arguments.length > 1 && arguments[1] !== void 0 ? arguments[1] : getCurrentInstanceName();
  const variantClasses = toRef(() => {
    const {
      variant
    } = toValue(props);
    return `${name}--variant-${variant}`;
  });
  const {
    colorClasses,
    colorStyles
  } = useColor(() => {
    const {
      variant,
      color
    } = toValue(props);
    return {
      [["elevated", "flat"].includes(variant) ? "background" : "text"]: color
    };
  });
  return {
    colorClasses,
    colorStyles,
    variantClasses
  };
}

export {
  genOverlays,
  makeVariantProps,
  useVariant
};
//# sourceMappingURL=chunk-AI2K6XWQ.js.map
