import {
  getCurrentInstanceName,
  propsFactory
} from "./chunk-QPYD2SUY.js";
import {
  toRef
} from "./chunk-ZS4T33C3.js";

// node_modules/vuetify/lib/composables/density.js
var allowedDensities = [null, "default", "comfortable", "compact"];
var makeDensityProps = propsFactory({
  density: {
    type: String,
    default: "default",
    validator: (v) => allowedDensities.includes(v)
  }
}, "density");
function useDensity(props) {
  let name = arguments.length > 1 && arguments[1] !== void 0 ? arguments[1] : getCurrentInstanceName();
  const densityClasses = toRef(() => {
    return `${name}--density-${props.density}`;
  });
  return {
    densityClasses
  };
}

export {
  makeDensityProps,
  useDensity
};
//# sourceMappingURL=chunk-BBMT6OHG.js.map
