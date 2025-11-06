import {
  VBtnToggleSymbol
} from "./chunk-XX2NLIHT.js";
import {
  makeGroupProps,
  useGroup
} from "./chunk-Y35KG7NE.js";
import {
  useSsrBoot
} from "./chunk-4R7HKKEK.js";
import {
  makeBorderProps,
  useBorder
} from "./chunk-VCJMQSPR.js";
import {
  makeElevationProps,
  useElevation
} from "./chunk-DA2W7YPQ.js";
import {
  makeRoundedProps,
  useRounded
} from "./chunk-2NAJM3NP.js";
import {
  useBackgroundColor
} from "./chunk-XL4SHUX5.js";
import {
  makeDensityProps,
  useDensity
} from "./chunk-BBMT6OHG.js";
import {
  makeTagProps
} from "./chunk-FSWG3OMG.js";
import {
  makeLayoutItemProps,
  useLayoutItem
} from "./chunk-UZFXVJER.js";
import {
  useProxiedModel
} from "./chunk-76O7CYX2.js";
import {
  makeThemeProps,
  useTheme
} from "./chunk-CEVB7HK3.js";
import {
  convertToUnit,
  genericComponent,
  makeComponentProps,
  propsFactory,
  provideDefaults,
  useRender
} from "./chunk-QPYD2SUY.js";
import {
  computed,
  createBaseVNode,
  createVNode,
  normalizeClass,
  normalizeStyle,
  toRef
} from "./chunk-ZS4T33C3.js";

// node_modules/vuetify/lib/components/VBottomNavigation/VBottomNavigation.js
import "/app/node_modules/vuetify/lib/components/VBottomNavigation/VBottomNavigation.css";
var makeVBottomNavigationProps = propsFactory({
  baseColor: String,
  bgColor: String,
  color: String,
  grow: Boolean,
  mode: {
    type: String,
    validator: (v) => !v || ["horizontal", "shift"].includes(v)
  },
  height: {
    type: [Number, String],
    default: 56
  },
  active: {
    type: Boolean,
    default: true
  },
  ...makeBorderProps(),
  ...makeComponentProps(),
  ...makeDensityProps(),
  ...makeElevationProps(),
  ...makeRoundedProps(),
  ...makeLayoutItemProps({
    name: "bottom-navigation"
  }),
  ...makeTagProps({
    tag: "header"
  }),
  ...makeGroupProps({
    selectedClass: "v-btn--selected"
  }),
  ...makeThemeProps()
}, "VBottomNavigation");
var VBottomNavigation = genericComponent()({
  name: "VBottomNavigation",
  props: makeVBottomNavigationProps(),
  emits: {
    "update:active": (value) => true,
    "update:modelValue": (value) => true
  },
  setup(props, _ref) {
    let {
      slots
    } = _ref;
    const {
      themeClasses
    } = useTheme();
    const {
      borderClasses
    } = useBorder(props);
    const {
      backgroundColorClasses,
      backgroundColorStyles
    } = useBackgroundColor(() => props.bgColor);
    const {
      densityClasses
    } = useDensity(props);
    const {
      elevationClasses
    } = useElevation(props);
    const {
      roundedClasses
    } = useRounded(props);
    const {
      ssrBootStyles
    } = useSsrBoot();
    const height = computed(() => Number(props.height) - (props.density === "comfortable" ? 8 : 0) - (props.density === "compact" ? 16 : 0));
    const isActive = useProxiedModel(props, "active", props.active);
    const {
      layoutItemStyles
    } = useLayoutItem({
      id: props.name,
      order: computed(() => parseInt(props.order, 10)),
      position: toRef(() => "bottom"),
      layoutSize: toRef(() => isActive.value ? height.value : 0),
      elementSize: height,
      active: isActive,
      absolute: toRef(() => props.absolute)
    });
    useGroup(props, VBtnToggleSymbol);
    provideDefaults({
      VBtn: {
        baseColor: toRef(() => props.baseColor),
        color: toRef(() => props.color),
        density: toRef(() => props.density),
        stacked: toRef(() => props.mode !== "horizontal"),
        variant: "text"
      }
    }, {
      scoped: true
    });
    useRender(() => {
      return createVNode(props.tag, {
        "class": normalizeClass(["v-bottom-navigation", {
          "v-bottom-navigation--active": isActive.value,
          "v-bottom-navigation--grow": props.grow,
          "v-bottom-navigation--shift": props.mode === "shift"
        }, themeClasses.value, backgroundColorClasses.value, borderClasses.value, densityClasses.value, elevationClasses.value, roundedClasses.value, props.class]),
        "style": normalizeStyle([backgroundColorStyles.value, layoutItemStyles.value, {
          height: convertToUnit(height.value)
        }, ssrBootStyles.value, props.style])
      }, {
        default: () => [slots.default && createBaseVNode("div", {
          "class": "v-bottom-navigation__content"
        }, [slots.default()])]
      });
    });
    return {};
  }
});

export {
  VBottomNavigation
};
//# sourceMappingURL=chunk-D2DY5FYY.js.map
