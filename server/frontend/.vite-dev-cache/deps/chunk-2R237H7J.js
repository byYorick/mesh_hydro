import {
  MaybeTransition,
  makeTransitionProps
} from "./chunk-EL5URPNZ.js";
import {
  makeLocationProps,
  useLocation
} from "./chunk-HCC5PZ53.js";
import {
  VIcon
} from "./chunk-KGUXEXV6.js";
import {
  makeDimensionProps,
  useDimension
} from "./chunk-WP3ESUMW.js";
import {
  makeRoundedProps,
  useRounded
} from "./chunk-2NAJM3NP.js";
import {
  makeTagProps
} from "./chunk-FSWG3OMG.js";
import {
  useBackgroundColor,
  useTextColor
} from "./chunk-XL4SHUX5.js";
import {
  IconValue
} from "./chunk-JBMQ7ZRV.js";
import {
  useLocale
} from "./chunk-TICGPC5M.js";
import {
  makeThemeProps,
  useTheme
} from "./chunk-CEVB7HK3.js";
import {
  genericComponent,
  makeComponentProps,
  pickWithRest,
  propsFactory,
  useRender
} from "./chunk-QPYD2SUY.js";
import {
  createBaseVNode,
  createVNode,
  mergeProps,
  vShow,
  withDirectives
} from "./chunk-ZS4T33C3.js";

// node_modules/vuetify/lib/components/VBadge/VBadge.js
import "/app/node_modules/vuetify/lib/components/VBadge/VBadge.css";
var makeVBadgeProps = propsFactory({
  bordered: Boolean,
  color: String,
  content: [Number, String],
  dot: Boolean,
  floating: Boolean,
  icon: IconValue,
  inline: Boolean,
  label: {
    type: String,
    default: "$vuetify.badge"
  },
  max: [Number, String],
  modelValue: {
    type: Boolean,
    default: true
  },
  offsetX: [Number, String],
  offsetY: [Number, String],
  textColor: String,
  ...makeComponentProps(),
  ...makeLocationProps({
    location: "top end"
  }),
  ...makeRoundedProps(),
  ...makeTagProps(),
  ...makeThemeProps(),
  ...makeTransitionProps({
    transition: "scale-rotate-transition"
  }),
  ...makeDimensionProps()
}, "VBadge");
var VBadge = genericComponent()({
  name: "VBadge",
  inheritAttrs: false,
  props: makeVBadgeProps(),
  setup(props, ctx) {
    const {
      backgroundColorClasses,
      backgroundColorStyles
    } = useBackgroundColor(() => props.color);
    const {
      roundedClasses
    } = useRounded(props);
    const {
      t
    } = useLocale();
    const {
      textColorClasses,
      textColorStyles
    } = useTextColor(() => props.textColor);
    const {
      themeClasses
    } = useTheme();
    const {
      locationStyles
    } = useLocation(props, true, (side) => {
      const base = props.floating ? props.dot ? 2 : 4 : props.dot ? 8 : 12;
      return base + (["top", "bottom"].includes(side) ? Number(props.offsetY ?? 0) : ["left", "right"].includes(side) ? Number(props.offsetX ?? 0) : 0);
    });
    const {
      dimensionStyles
    } = useDimension(props);
    useRender(() => {
      const value = Number(props.content);
      const content = !props.max || isNaN(value) ? props.content : value <= Number(props.max) ? value : `${props.max}+`;
      const [badgeAttrs, attrs] = pickWithRest(ctx.attrs, ["aria-atomic", "aria-label", "aria-live", "role", "title"]);
      return createVNode(props.tag, mergeProps({
        "class": ["v-badge", {
          "v-badge--bordered": props.bordered,
          "v-badge--dot": props.dot,
          "v-badge--floating": props.floating,
          "v-badge--inline": props.inline
        }, props.class]
      }, attrs, {
        "style": props.style
      }), {
        default: () => {
          var _a, _b;
          return [createBaseVNode("div", {
            "class": "v-badge__wrapper"
          }, [(_b = (_a = ctx.slots).default) == null ? void 0 : _b.call(_a), createVNode(MaybeTransition, {
            "transition": props.transition
          }, {
            default: () => {
              var _a2, _b2;
              return [withDirectives(createBaseVNode("span", mergeProps({
                "class": ["v-badge__badge", themeClasses.value, backgroundColorClasses.value, roundedClasses.value, textColorClasses.value],
                "style": [backgroundColorStyles.value, textColorStyles.value, dimensionStyles.value, props.inline ? {} : locationStyles.value],
                "aria-atomic": "true",
                "aria-label": t(props.label, value),
                "aria-live": "polite",
                "role": "status"
              }, badgeAttrs), [props.dot ? void 0 : ctx.slots.badge ? (_b2 = (_a2 = ctx.slots).badge) == null ? void 0 : _b2.call(_a2) : props.icon ? createVNode(VIcon, {
                "icon": props.icon
              }, null) : content]), [[vShow, props.modelValue]])];
            }
          })])];
        }
      });
    });
    return {};
  }
});

export {
  VBadge
};
//# sourceMappingURL=chunk-2R237H7J.js.map
