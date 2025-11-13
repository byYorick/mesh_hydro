import {
  makePositionProps,
  usePosition
} from "./chunk-WFBHAYLZ.js";
import {
  makeElevationProps,
  useElevation
} from "./chunk-7HWKLQOM.js";
import {
  makeBorderProps,
  useBorder
} from "./chunk-LS7KDB5J.js";
import {
  makeLocationProps,
  useLocation
} from "./chunk-L2SDK2CV.js";
import {
  makeRoundedProps,
  useRounded
} from "./chunk-X772MWO6.js";
import {
  makeTagProps
} from "./chunk-UVMYLE7E.js";
import {
  makeDimensionProps,
  useDimension
} from "./chunk-TNSFOAMR.js";
import {
  useBackgroundColor
} from "./chunk-Q6KC3LXY.js";
import {
  makeThemeProps,
  provideTheme
} from "./chunk-YBZL2HCC.js";
import {
  genericComponent,
  makeComponentProps,
  propsFactory,
  useRender
} from "./chunk-I5IG4TPY.js";
import {
  createVNode,
  normalizeClass,
  normalizeStyle
} from "./chunk-ZS4T33C3.js";

// node_modules/vuetify/lib/components/VSheet/VSheet.js
import "/app/node_modules/vuetify/lib/components/VSheet/VSheet.css";
var makeVSheetProps = propsFactory({
  color: String,
  ...makeBorderProps(),
  ...makeComponentProps(),
  ...makeDimensionProps(),
  ...makeElevationProps(),
  ...makeLocationProps(),
  ...makePositionProps(),
  ...makeRoundedProps(),
  ...makeTagProps(),
  ...makeThemeProps()
}, "VSheet");
var VSheet = genericComponent()({
  name: "VSheet",
  props: makeVSheetProps(),
  setup(props, _ref) {
    let {
      slots
    } = _ref;
    const {
      themeClasses
    } = provideTheme(props);
    const {
      backgroundColorClasses,
      backgroundColorStyles
    } = useBackgroundColor(() => props.color);
    const {
      borderClasses
    } = useBorder(props);
    const {
      dimensionStyles
    } = useDimension(props);
    const {
      elevationClasses
    } = useElevation(props);
    const {
      locationStyles
    } = useLocation(props);
    const {
      positionClasses
    } = usePosition(props);
    const {
      roundedClasses
    } = useRounded(props);
    useRender(() => createVNode(props.tag, {
      "class": normalizeClass(["v-sheet", themeClasses.value, backgroundColorClasses.value, borderClasses.value, elevationClasses.value, positionClasses.value, roundedClasses.value, props.class]),
      "style": normalizeStyle([backgroundColorStyles.value, dimensionStyles.value, locationStyles.value, props.style])
    }, slots));
    return {};
  }
});

export {
  makeVSheetProps,
  VSheet
};
//# sourceMappingURL=chunk-O6L4IA25.js.map
