import {
  makeElevationProps,
  useElevation
} from "./chunk-DA2W7YPQ.js";
import {
  VDefaultsProvider
} from "./chunk-7L4ZWQEP.js";
import {
  VIcon
} from "./chunk-KGUXEXV6.js";
import {
  makeSizeProps,
  useSize
} from "./chunk-2UL4C4D2.js";
import {
  makeDimensionProps,
  useDimension
} from "./chunk-WP3ESUMW.js";
import {
  makeDensityProps,
  useDensity
} from "./chunk-BBMT6OHG.js";
import {
  makeRoundedProps,
  useRounded
} from "./chunk-2NAJM3NP.js";
import {
  makeTagProps
} from "./chunk-FSWG3OMG.js";
import {
  useBackgroundColor
} from "./chunk-XL4SHUX5.js";
import {
  IconValue
} from "./chunk-JBMQ7ZRV.js";
import {
  useRtl
} from "./chunk-TICGPC5M.js";
import {
  makeThemeProps,
  provideTheme
} from "./chunk-CEVB7HK3.js";
import {
  convertToUnit,
  genericComponent,
  makeComponentProps,
  pick,
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
  ref,
  shallowRef,
  toRef,
  watch
} from "./chunk-ZS4T33C3.js";

// node_modules/vuetify/lib/components/VTimeline/VTimeline.js
import "/app/node_modules/vuetify/lib/components/VTimeline/VTimeline.css";

// node_modules/vuetify/lib/components/VTimeline/VTimelineDivider.js
var makeVTimelineDividerProps = propsFactory({
  dotColor: String,
  fillDot: Boolean,
  hideDot: Boolean,
  icon: IconValue,
  iconColor: String,
  lineColor: String,
  ...makeComponentProps(),
  ...makeRoundedProps(),
  ...makeSizeProps(),
  ...makeElevationProps()
}, "VTimelineDivider");
var VTimelineDivider = genericComponent()({
  name: "VTimelineDivider",
  props: makeVTimelineDividerProps(),
  setup(props, _ref) {
    let {
      slots
    } = _ref;
    const {
      sizeClasses,
      sizeStyles
    } = useSize(props, "v-timeline-divider__dot");
    const {
      backgroundColorStyles,
      backgroundColorClasses
    } = useBackgroundColor(() => props.dotColor);
    const {
      roundedClasses
    } = useRounded(props, "v-timeline-divider__dot");
    const {
      elevationClasses
    } = useElevation(props);
    const {
      backgroundColorClasses: lineColorClasses,
      backgroundColorStyles: lineColorStyles
    } = useBackgroundColor(() => props.lineColor);
    useRender(() => createBaseVNode("div", {
      "class": normalizeClass(["v-timeline-divider", {
        "v-timeline-divider--fill-dot": props.fillDot
      }, props.class]),
      "style": normalizeStyle(props.style)
    }, [createBaseVNode("div", {
      "class": normalizeClass(["v-timeline-divider__before", lineColorClasses.value]),
      "style": normalizeStyle(lineColorStyles.value)
    }, null), !props.hideDot && createBaseVNode("div", {
      "key": "dot",
      "class": normalizeClass(["v-timeline-divider__dot", elevationClasses.value, roundedClasses.value, sizeClasses.value]),
      "style": normalizeStyle(sizeStyles.value)
    }, [createBaseVNode("div", {
      "class": normalizeClass(["v-timeline-divider__inner-dot", backgroundColorClasses.value, roundedClasses.value]),
      "style": normalizeStyle(backgroundColorStyles.value)
    }, [!slots.default ? createVNode(VIcon, {
      "key": "icon",
      "color": props.iconColor,
      "icon": props.icon,
      "size": props.size
    }, null) : createVNode(VDefaultsProvider, {
      "key": "icon-defaults",
      "disabled": !props.icon,
      "defaults": {
        VIcon: {
          color: props.iconColor,
          icon: props.icon,
          size: props.size
        }
      }
    }, slots.default)])]), createBaseVNode("div", {
      "class": normalizeClass(["v-timeline-divider__after", lineColorClasses.value]),
      "style": normalizeStyle(lineColorStyles.value)
    }, null)]));
    return {};
  }
});

// node_modules/vuetify/lib/components/VTimeline/VTimelineItem.js
var makeVTimelineItemProps = propsFactory({
  density: String,
  dotColor: String,
  fillDot: Boolean,
  hideDot: Boolean,
  hideOpposite: {
    type: Boolean,
    default: void 0
  },
  icon: IconValue,
  iconColor: String,
  lineInset: [Number, String],
  side: {
    type: String,
    validator: (v) => v == null || ["start", "end"].includes(v)
  },
  ...makeComponentProps(),
  ...makeDimensionProps(),
  ...makeElevationProps(),
  ...makeRoundedProps(),
  ...makeSizeProps(),
  ...makeTagProps()
}, "VTimelineItem");
var VTimelineItem = genericComponent()({
  name: "VTimelineItem",
  props: makeVTimelineItemProps(),
  setup(props, _ref) {
    let {
      slots
    } = _ref;
    const {
      dimensionStyles
    } = useDimension(props);
    const dotSize = shallowRef(0);
    const dotRef = ref();
    watch(dotRef, (newValue) => {
      var _a;
      if (!newValue) return;
      dotSize.value = ((_a = newValue.$el.querySelector(".v-timeline-divider__dot")) == null ? void 0 : _a.getBoundingClientRect().width) ?? 0;
    }, {
      flush: "post"
    });
    useRender(() => {
      var _a, _b;
      return createBaseVNode("div", {
        "class": normalizeClass(["v-timeline-item", {
          "v-timeline-item--fill-dot": props.fillDot,
          "v-timeline-item--side-start": props.side === "start",
          "v-timeline-item--side-end": props.side === "end"
        }, props.class]),
        "style": normalizeStyle([{
          "--v-timeline-dot-size": convertToUnit(dotSize.value),
          "--v-timeline-line-inset": props.lineInset ? `calc(var(--v-timeline-dot-size) / 2 + ${convertToUnit(props.lineInset)})` : convertToUnit(0)
        }, props.style])
      }, [createBaseVNode("div", {
        "class": "v-timeline-item__body",
        "style": normalizeStyle(dimensionStyles.value)
      }, [(_a = slots.default) == null ? void 0 : _a.call(slots)]), createVNode(VTimelineDivider, {
        "ref": dotRef,
        "hideDot": props.hideDot,
        "icon": props.icon,
        "iconColor": props.iconColor,
        "size": props.size,
        "elevation": props.elevation,
        "dotColor": props.dotColor,
        "fillDot": props.fillDot,
        "rounded": props.rounded
      }, {
        default: slots.icon
      }), props.density !== "compact" && createBaseVNode("div", {
        "class": "v-timeline-item__opposite"
      }, [!props.hideOpposite && ((_b = slots.opposite) == null ? void 0 : _b.call(slots))])]);
    });
    return {};
  }
});

// node_modules/vuetify/lib/components/VTimeline/VTimeline.js
var makeVTimelineProps = propsFactory({
  align: {
    type: String,
    default: "center",
    validator: (v) => ["center", "start"].includes(v)
  },
  direction: {
    type: String,
    default: "vertical",
    validator: (v) => ["vertical", "horizontal"].includes(v)
  },
  justify: {
    type: String,
    default: "auto",
    validator: (v) => ["auto", "center"].includes(v)
  },
  side: {
    type: String,
    validator: (v) => v == null || ["start", "end"].includes(v)
  },
  lineThickness: {
    type: [String, Number],
    default: 2
  },
  lineColor: String,
  truncateLine: {
    type: String,
    validator: (v) => ["start", "end", "both"].includes(v)
  },
  ...pick(makeVTimelineItemProps({
    lineInset: 0
  }), ["dotColor", "fillDot", "hideOpposite", "iconColor", "lineInset", "size"]),
  ...makeComponentProps(),
  ...makeDensityProps(),
  ...makeTagProps(),
  ...makeThemeProps()
}, "VTimeline");
var VTimeline = genericComponent()({
  name: "VTimeline",
  props: makeVTimelineProps(),
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
      rtlClasses
    } = useRtl();
    provideDefaults({
      VTimelineDivider: {
        lineColor: toRef(() => props.lineColor)
      },
      VTimelineItem: {
        density: toRef(() => props.density),
        dotColor: toRef(() => props.dotColor),
        fillDot: toRef(() => props.fillDot),
        hideOpposite: toRef(() => props.hideOpposite),
        iconColor: toRef(() => props.iconColor),
        lineColor: toRef(() => props.lineColor),
        lineInset: toRef(() => props.lineInset),
        size: toRef(() => props.size)
      }
    });
    const sideClasses = computed(() => {
      const side = props.side ? props.side : props.density !== "default" ? "end" : null;
      return side && `v-timeline--side-${side}`;
    });
    const truncateClasses = computed(() => {
      const classes = ["v-timeline--truncate-line-start", "v-timeline--truncate-line-end"];
      switch (props.truncateLine) {
        case "both":
          return classes;
        case "start":
          return classes[0];
        case "end":
          return classes[1];
        default:
          return null;
      }
    });
    useRender(() => createVNode(props.tag, {
      "class": normalizeClass(["v-timeline", `v-timeline--${props.direction}`, `v-timeline--align-${props.align}`, `v-timeline--justify-${props.justify}`, truncateClasses.value, {
        "v-timeline--inset-line": !!props.lineInset
      }, themeClasses.value, densityClasses.value, sideClasses.value, rtlClasses.value, props.class]),
      "style": normalizeStyle([{
        "--v-timeline-line-thickness": convertToUnit(props.lineThickness)
      }, props.style])
    }, slots));
    return {};
  }
});

export {
  VTimelineItem,
  VTimeline
};
//# sourceMappingURL=chunk-VGU3WQ6F.js.map
