import {
  createLayout,
  makeLayoutProps
} from "./chunk-UZFXVJER.js";
import {
  useRtl
} from "./chunk-TICGPC5M.js";
import {
  makeThemeProps,
  provideTheme
} from "./chunk-CEVB7HK3.js";
import {
  genericComponent,
  makeComponentProps,
  omit,
  propsFactory,
  useRender
} from "./chunk-QPYD2SUY.js";
import {
  createBaseVNode,
  normalizeClass,
  normalizeStyle
} from "./chunk-ZS4T33C3.js";

// node_modules/vuetify/lib/components/VApp/VApp.js
import "/app/node_modules/vuetify/lib/components/VApp/VApp.css";
var makeVAppProps = propsFactory({
  ...makeComponentProps(),
  ...omit(makeLayoutProps(), ["fullHeight"]),
  ...makeThemeProps()
}, "VApp");
var VApp = genericComponent()({
  name: "VApp",
  props: makeVAppProps(),
  setup(props, _ref) {
    let {
      slots
    } = _ref;
    const theme = provideTheme(props);
    const {
      layoutClasses,
      getLayoutItem,
      items,
      layoutRef
    } = createLayout({
      ...props,
      fullHeight: true
    });
    const {
      rtlClasses
    } = useRtl();
    useRender(() => {
      var _a;
      return createBaseVNode("div", {
        "ref": layoutRef,
        "class": normalizeClass(["v-application", theme.themeClasses.value, layoutClasses.value, rtlClasses.value, props.class]),
        "style": normalizeStyle([props.style])
      }, [createBaseVNode("div", {
        "class": "v-application__wrap"
      }, [(_a = slots.default) == null ? void 0 : _a.call(slots)])]);
    });
    return {
      getLayoutItem,
      items,
      theme
    };
  }
});

export {
  VApp
};
//# sourceMappingURL=chunk-OG7NPJAR.js.map
