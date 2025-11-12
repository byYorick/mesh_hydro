import {
  VSelectionControl,
  VSelectionControlGroup,
  makeSelectionControlGroupProps
} from "./chunk-NWW3IG4H.js";
import {
  VInput,
  makeVInputProps
} from "./chunk-2ZUUQFWX.js";
import {
  VLabel
} from "./chunk-T3H6SUKO.js";
import {
  forwardRefs
} from "./chunk-QJKQQOXD.js";
import {
  IconValue
} from "./chunk-JBMQ7ZRV.js";
import {
  useProxiedModel
} from "./chunk-76O7CYX2.js";
import {
  filterInputAttrs,
  genericComponent,
  omit,
  propsFactory,
  useRender
} from "./chunk-QPYD2SUY.js";
import {
  Fragment,
  computed,
  createBaseVNode,
  createVNode,
  mergeProps,
  ref,
  useId
} from "./chunk-ZS4T33C3.js";

// node_modules/vuetify/lib/components/VRadioGroup/VRadioGroup.js
import "/app/node_modules/vuetify/lib/components/VRadioGroup/VRadioGroup.css";
var makeVRadioGroupProps = propsFactory({
  height: {
    type: [Number, String],
    default: "auto"
  },
  ...makeVInputProps(),
  ...omit(makeSelectionControlGroupProps(), ["multiple"]),
  trueIcon: {
    type: IconValue,
    default: "$radioOn"
  },
  falseIcon: {
    type: IconValue,
    default: "$radioOff"
  },
  type: {
    type: String,
    default: "radio"
  }
}, "VRadioGroup");
var VRadioGroup = genericComponent()({
  name: "VRadioGroup",
  inheritAttrs: false,
  props: makeVRadioGroupProps(),
  emits: {
    "update:modelValue": (value) => true
  },
  setup(props, _ref) {
    let {
      attrs,
      slots
    } = _ref;
    const uid = useId();
    const id = computed(() => props.id || `radio-group-${uid}`);
    const model = useProxiedModel(props, "modelValue");
    const inputRef = ref();
    useRender(() => {
      const [rootAttrs, controlAttrs] = filterInputAttrs(attrs);
      const inputProps = VInput.filterProps(props);
      const controlProps = VSelectionControl.filterProps(props);
      const label = slots.label ? slots.label({
        label: props.label,
        props: {
          for: id.value
        }
      }) : props.label;
      return createVNode(VInput, mergeProps({
        "ref": inputRef,
        "class": ["v-radio-group", props.class],
        "style": props.style
      }, rootAttrs, inputProps, {
        "modelValue": model.value,
        "onUpdate:modelValue": ($event) => model.value = $event,
        "id": id.value
      }), {
        ...slots,
        default: (_ref2) => {
          let {
            id: id2,
            messagesId,
            isDisabled,
            isReadonly
          } = _ref2;
          return createBaseVNode(Fragment, null, [label && createVNode(VLabel, {
            "id": id2.value
          }, {
            default: () => [label]
          }), createVNode(VSelectionControlGroup, mergeProps(controlProps, {
            "id": id2.value,
            "aria-describedby": messagesId.value,
            "defaultsTarget": "VRadio",
            "trueIcon": props.trueIcon,
            "falseIcon": props.falseIcon,
            "type": props.type,
            "disabled": isDisabled.value,
            "readonly": isReadonly.value,
            "aria-labelledby": label ? id2.value : void 0,
            "multiple": false
          }, controlAttrs, {
            "modelValue": model.value,
            "onUpdate:modelValue": ($event) => model.value = $event
          }), slots)]);
        }
      });
    });
    return forwardRefs({}, inputRef);
  }
});

export {
  VRadioGroup
};
//# sourceMappingURL=chunk-RC6KTZXD.js.map
