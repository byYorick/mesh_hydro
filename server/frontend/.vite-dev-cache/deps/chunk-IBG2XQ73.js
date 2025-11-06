import {
  VSlideGroup,
  makeVSlideGroupProps
} from "./chunk-CP7I7NF3.js";
import {
  makeGroupProps,
  useGroup
} from "./chunk-Y35KG7NE.js";
import {
  makeVariantProps
} from "./chunk-AI2K6XWQ.js";
import {
  makeTagProps
} from "./chunk-FSWG3OMG.js";
import {
  makeThemeProps,
  provideTheme
} from "./chunk-CEVB7HK3.js";
import {
  deepEqual,
  genericComponent,
  makeComponentProps,
  propsFactory,
  provideDefaults,
  useRender
} from "./chunk-QPYD2SUY.js";
import {
  createVNode,
  mergeProps,
  toRef
} from "./chunk-ZS4T33C3.js";

// node_modules/vuetify/lib/components/VChipGroup/VChipGroup.js
import "/app/node_modules/vuetify/lib/components/VChipGroup/VChipGroup.css";
var VChipGroupSymbol = Symbol.for("vuetify:v-chip-group");
var makeVChipGroupProps = propsFactory({
  baseColor: String,
  column: Boolean,
  filter: Boolean,
  valueComparator: {
    type: Function,
    default: deepEqual
  },
  ...makeVSlideGroupProps(),
  ...makeComponentProps(),
  ...makeGroupProps({
    selectedClass: "v-chip--selected"
  }),
  ...makeTagProps(),
  ...makeThemeProps(),
  ...makeVariantProps({
    variant: "tonal"
  })
}, "VChipGroup");
var VChipGroup = genericComponent()({
  name: "VChipGroup",
  props: makeVChipGroupProps(),
  emits: {
    "update:modelValue": (value) => true
  },
  setup(props, _ref) {
    let {
      slots
    } = _ref;
    const {
      themeClasses
    } = provideTheme(props);
    const {
      isSelected,
      select,
      next,
      prev,
      selected
    } = useGroup(props, VChipGroupSymbol);
    provideDefaults({
      VChip: {
        baseColor: toRef(() => props.baseColor),
        color: toRef(() => props.color),
        disabled: toRef(() => props.disabled),
        filter: toRef(() => props.filter),
        variant: toRef(() => props.variant)
      }
    });
    useRender(() => {
      const slideGroupProps = VSlideGroup.filterProps(props);
      return createVNode(VSlideGroup, mergeProps(slideGroupProps, {
        "class": ["v-chip-group", {
          "v-chip-group--column": props.column
        }, themeClasses.value, props.class],
        "style": props.style
      }), {
        default: () => {
          var _a;
          return [(_a = slots.default) == null ? void 0 : _a.call(slots, {
            isSelected,
            select,
            next,
            prev,
            selected: selected.value
          })];
        }
      });
    });
    return {};
  }
});

export {
  VChipGroupSymbol,
  VChipGroup
};
//# sourceMappingURL=chunk-IBG2XQ73.js.map
