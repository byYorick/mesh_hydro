import {
  makeDensityProps,
  useDensity
} from "./chunk-BBMT6OHG.js";
import {
  makeTagProps
} from "./chunk-FSWG3OMG.js";
import {
  makeThemeProps,
  provideTheme
} from "./chunk-CEVB7HK3.js";
import {
  convertToUnit,
  genericComponent,
  makeComponentProps,
  propsFactory,
  useRender
} from "./chunk-QPYD2SUY.js";
import {
  createBaseVNode,
  createVNode,
  normalizeClass,
  normalizeStyle
} from "./chunk-ZS4T33C3.js";

// node_modules/vuetify/lib/components/VTable/VTable.js
import "/app/node_modules/vuetify/lib/components/VTable/VTable.css";
var makeVTableProps = propsFactory({
  fixedHeader: Boolean,
  fixedFooter: Boolean,
  height: [Number, String],
  hover: Boolean,
  striped: {
    type: String,
    default: null,
    validator: (v) => ["even", "odd"].includes(v)
  },
  ...makeComponentProps(),
  ...makeDensityProps(),
  ...makeTagProps(),
  ...makeThemeProps()
}, "VTable");
var VTable = genericComponent()({
  name: "VTable",
  props: makeVTableProps(),
  setup(props, _ref) {
    let {
      slots,
      emit
    } = _ref;
    const {
      themeClasses
    } = provideTheme(props);
    const {
      densityClasses
    } = useDensity(props);
    useRender(() => createVNode(props.tag, {
      "class": normalizeClass(["v-table", {
        "v-table--fixed-height": !!props.height,
        "v-table--fixed-header": props.fixedHeader,
        "v-table--fixed-footer": props.fixedFooter,
        "v-table--has-top": !!slots.top,
        "v-table--has-bottom": !!slots.bottom,
        "v-table--hover": props.hover,
        "v-table--striped-even": props.striped === "even",
        "v-table--striped-odd": props.striped === "odd"
      }, themeClasses.value, densityClasses.value, props.class]),
      "style": normalizeStyle(props.style)
    }, {
      default: () => {
        var _a, _b, _c;
        return [(_a = slots.top) == null ? void 0 : _a.call(slots), slots.default ? createBaseVNode("div", {
          "class": "v-table__wrapper",
          "style": {
            height: convertToUnit(props.height)
          }
        }, [createBaseVNode("table", null, [slots.default()])]) : (_b = slots.wrapper) == null ? void 0 : _b.call(slots), (_c = slots.bottom) == null ? void 0 : _c.call(slots)];
      }
    }));
    return {};
  }
});

export {
  makeVTableProps,
  VTable
};
//# sourceMappingURL=chunk-ZXHJRP63.js.map
