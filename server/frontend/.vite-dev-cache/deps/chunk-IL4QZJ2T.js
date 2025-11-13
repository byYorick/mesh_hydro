import {
  makeElevationProps,
  useElevation
} from "./chunk-7HWKLQOM.js";
import {
  makeDensityProps,
  useDensity
} from "./chunk-B7YG57E2.js";
import {
  makeBorderProps,
  useBorder
} from "./chunk-LS7KDB5J.js";
import {
  makeVariantProps
} from "./chunk-MYBNBMBC.js";
import {
  makeRoundedProps,
  useRounded
} from "./chunk-X772MWO6.js";
import {
  makeGroupProps,
  useGroup
} from "./chunk-TGCHCPFR.js";
import {
  makeTagProps
} from "./chunk-UVMYLE7E.js";
import {
  makeThemeProps,
  provideTheme
} from "./chunk-YBZL2HCC.js";
import {
  genericComponent,
  makeComponentProps,
  propsFactory,
  provideDefaults,
  useRender
} from "./chunk-I5IG4TPY.js";
import {
  createVNode,
  mergeProps,
  normalizeClass,
  normalizeStyle,
  toRef
} from "./chunk-ZS4T33C3.js";

// node_modules/vuetify/lib/components/VBtnGroup/VBtnGroup.js
import "/app/node_modules/vuetify/lib/components/VBtnGroup/VBtnGroup.css";
var makeVBtnGroupProps = propsFactory({
  baseColor: String,
  divided: Boolean,
  direction: {
    type: String,
    default: "horizontal"
  },
  ...makeBorderProps(),
  ...makeComponentProps(),
  ...makeDensityProps(),
  ...makeElevationProps(),
  ...makeRoundedProps(),
  ...makeTagProps(),
  ...makeThemeProps(),
  ...makeVariantProps()
}, "VBtnGroup");
var VBtnGroup = genericComponent()({
  name: "VBtnGroup",
  props: makeVBtnGroupProps(),
  setup(props, _ref) {
    let {
      slots
    } = _ref;
    const {
      themeClasses
    } = provideTheme(props);
    const {
      densityClasses
    } = useDensity(props);
    const {
      borderClasses
    } = useBorder(props);
    const {
      elevationClasses
    } = useElevation(props);
    const {
      roundedClasses
    } = useRounded(props);
    provideDefaults({
      VBtn: {
        height: toRef(() => props.direction === "horizontal" ? "auto" : null),
        baseColor: toRef(() => props.baseColor),
        color: toRef(() => props.color),
        density: toRef(() => props.density),
        flat: true,
        variant: toRef(() => props.variant)
      }
    });
    useRender(() => {
      return createVNode(props.tag, {
        "class": normalizeClass(["v-btn-group", `v-btn-group--${props.direction}`, {
          "v-btn-group--divided": props.divided
        }, themeClasses.value, borderClasses.value, densityClasses.value, elevationClasses.value, roundedClasses.value, props.class]),
        "style": normalizeStyle(props.style)
      }, slots);
    });
  }
});

// node_modules/vuetify/lib/components/VBtnToggle/VBtnToggle.js
import "/app/node_modules/vuetify/lib/components/VBtnToggle/VBtnToggle.css";
var VBtnToggleSymbol = Symbol.for("vuetify:v-btn-toggle");
var makeVBtnToggleProps = propsFactory({
  ...makeVBtnGroupProps(),
  ...makeGroupProps()
}, "VBtnToggle");
var VBtnToggle = genericComponent()({
  name: "VBtnToggle",
  props: makeVBtnToggleProps(),
  emits: {
    "update:modelValue": (value) => true
  },
  setup(props, _ref) {
    let {
      slots
    } = _ref;
    const {
      isSelected,
      next,
      prev,
      select,
      selected
    } = useGroup(props, VBtnToggleSymbol);
    useRender(() => {
      const btnGroupProps = VBtnGroup.filterProps(props);
      return createVNode(VBtnGroup, mergeProps({
        "class": ["v-btn-toggle", props.class]
      }, btnGroupProps, {
        "style": props.style
      }), {
        default: () => {
          var _a;
          return [(_a = slots.default) == null ? void 0 : _a.call(slots, {
            isSelected,
            next,
            prev,
            select,
            selected
          })];
        }
      });
    });
    return {
      next,
      prev,
      select
    };
  }
});

export {
  VBtnGroup,
  VBtnToggleSymbol,
  VBtnToggle
};
//# sourceMappingURL=chunk-IL4QZJ2T.js.map
