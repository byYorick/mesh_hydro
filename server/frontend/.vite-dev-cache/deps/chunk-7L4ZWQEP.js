import {
  genericComponent,
  propsFactory,
  provideDefaults
} from "./chunk-QPYD2SUY.js";
import {
  toRefs
} from "./chunk-ZS4T33C3.js";

// node_modules/vuetify/lib/components/VDefaultsProvider/VDefaultsProvider.js
var makeVDefaultsProviderProps = propsFactory({
  defaults: Object,
  disabled: Boolean,
  reset: [Number, String],
  root: [Boolean, String],
  scoped: Boolean
}, "VDefaultsProvider");
var VDefaultsProvider = genericComponent(false)({
  name: "VDefaultsProvider",
  props: makeVDefaultsProviderProps(),
  setup(props, _ref) {
    let {
      slots
    } = _ref;
    const {
      defaults,
      disabled,
      reset,
      root,
      scoped
    } = toRefs(props);
    provideDefaults(defaults, {
      reset,
      root,
      scoped,
      disabled
    });
    return () => {
      var _a;
      return (_a = slots.default) == null ? void 0 : _a.call(slots);
    };
  }
});

export {
  VDefaultsProvider
};
//# sourceMappingURL=chunk-7L4ZWQEP.js.map
