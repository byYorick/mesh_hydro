import {
  makeLayoutItemProps,
  useLayoutItem
} from "./chunk-DPWEIKVG.js";
import {
  VBtnToggleSymbol
} from "./chunk-5RDULKVX.js";
import {
  useSsrBoot
} from "./chunk-4R7HKKEK.js";
import {
  makeGroupProps,
  useGroup
} from "./chunk-TGCHCPFR.js";
import {
  makeElevationProps,
  useElevation
} from "./chunk-7HWKLQOM.js";
import {
  makeBorderProps,
  useBorder
} from "./chunk-LS7KDB5J.js";
import {
  makeDensityProps,
  useDensity
} from "./chunk-B7YG57E2.js";
import {
  makeRoundedProps,
  useRounded
} from "./chunk-X772MWO6.js";
import {
  makeTagProps
} from "./chunk-UVMYLE7E.js";
import {
  useBackgroundColor
} from "./chunk-Q6KC3LXY.js";
import {
  makeThemeProps,
  useTheme
} from "./chunk-YBZL2HCC.js";
import {
  useProxiedModel
} from "./chunk-7GPVUA6G.js";
import {
  convertToUnit,
  genericComponent,
  makeComponentProps,
  propsFactory,
  provideDefaults,
  useRender
} from "./chunk-I5IG4TPY.js";
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
//# sourceMappingURL=chunk-YNCOOZJS.js.map
